#include <FL/Fl.H>
#include <FL/Fl_Window.H>
#include <FL/Fl_Text_Editor.H>
#include <FL/Fl_Text_Buffer.H>
#include <FL/Fl_Menu_Bar.H>
#include <FL/Fl_File_Chooser.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Input.H>
#include <FL/Fl_Box.H>
#include <FL/fl_ask.H>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <cstdlib>
#include <cctype>

class EditorWindow : public Fl_Window {
private:
    Fl_Text_Editor* editor;
    Fl_Text_Buffer* textbuf;
    Fl_Text_Buffer* stylebuf;
    Fl_Menu_Bar* menu;

    // For search/replace dialogs:
    Fl_Window* search_dialog = nullptr;
    Fl_Input* search_input = nullptr;
    char* search_string = nullptr;

    // Syntax highlighting style table (6 entries):
    Fl_Text_Display::Style_Table_Entry styletable[6] = {
        { FL_BLACK,      FL_COURIER,        14 }, // Plain
        { FL_DARK_GREEN, FL_COURIER_BOLD,   14 }, // Keywords
        { FL_BLUE,       FL_COURIER,        14 }, // Comments
        { FL_RED,        FL_COURIER,        14 }, // Strings
        { FL_DARK_RED,   FL_COURIER_BOLD,   14 }, // Functions
        { FL_DARK_BLUE,  FL_COURIER_BOLD,   14 }  // Numbers
    };

    // Modification tracking for file saving:
    bool is_modified = false;
    std::string current_file;

    // For manual undo/redo: we keep snapshots of the text.
    std::vector<std::string> undo_stack;
    std::vector<std::string> redo_stack;
    std::string last_snapshot;   // last known state
    bool in_undo_redo = false;

    // --- Menu callback dispatch ---
    static void menu_callback(Fl_Widget* w, void* data) {
        EditorWindow* win = (EditorWindow*)data;
        const Fl_Menu_Item* m = ((Fl_Menu_Bar*)w)->mvalue();
        if (!m) return;
        const char* label = m->label();

        // Use substring matching to catch labels with submenu prefixes
        if (strstr(label, "Open") != nullptr) {
            win->open_file();
        } else if (strstr(label, "Save") != nullptr) {
            win->save_file();
        } else if (strstr(label, "Find") != nullptr) {
            win->show_search_dialog();
        } else if (strstr(label, "Replace") != nullptr) {
            win->replace_text();
        } else if (strstr(label, "Undo") != nullptr) {
            win->undo_action();
        } else if (strstr(label, "Redo") != nullptr) {
            win->redo_action();
        } else if (strstr(label, "Font Size") != nullptr) {
            win->change_font_size();
        } else if (strstr(label, "Font Style") != nullptr) {
            win->change_font_style();
        } else if (strstr(label, "About") != nullptr) {
            fl_message("Personal Text Editor\nVersion 1.0\n By Ajeya Kumara!");
        } else if (strstr(label, "Quit") != nullptr) {
            if (win->check_save()) win->hide();
        }
    }

    bool check_save() {
        if (is_modified) {
            int choice = fl_choice("The current file has unsaved changes.\n"
                                   "Would you like to save it?",
                                   "Cancel", "Save", "Don't Save");
            if (choice == 0) return false;
            if (choice == 1) {
                save_file();
            }
        }
        return true;
    }

    void open_file() {
        if (!check_save()) return;
        Fl_File_Chooser chooser(".", "Text Files (*.txt)\tAll Files (*.*)",
                                  Fl_File_Chooser::SINGLE, "Open File");
        chooser.show();
        while (chooser.shown()) Fl::wait();
        if (chooser.value() != nullptr) {
            std::ifstream file(chooser.value());
            if (file) {
                std::stringstream buffer;
                buffer << file.rdbuf();
                textbuf->text(buffer.str().c_str());
                current_file = chooser.value();
                is_modified = false;
                // Reset undo/redo history:
                undo_stack.clear();
                redo_stack.clear();
                last_snapshot = textbuf->text();
                update_title();
            } else {
                fl_alert("Error reading from file '%s'.", chooser.value());
            }
        }
    }

    void save_file() {
        if (current_file.empty()) {
            Fl_File_Chooser chooser(".", "Text Files (*.txt)\tAll Files (*.*)",
                                      Fl_File_Chooser::CREATE, "Save File");
            chooser.show();
            while (chooser.shown()) Fl::wait();
            if (chooser.value() != nullptr && strlen(chooser.value()) > 0) {
                current_file = chooser.value();
            } else {
                const char* fname = fl_input("Enter file name:", "Untitled.txt");
                if(fname)
                    current_file = fname;
                else
                    return;
            }
        }
        std::ofstream file(current_file);
        if (file) {
            file << textbuf->text();
            is_modified = false;
            update_title();
        } else {
            fl_alert("Error writing to file '%s'.", current_file.c_str());
        }
    }

    void update_title() {
        char title[256];
        sprintf(title, "FLTK Text Editor - %s%s",
                current_file.empty() ? "Untitled" : current_file.c_str(),
                is_modified ? " (modified)" : "");
        label(title);
    }

    // --- Search / Replace functions ---
    void show_search_dialog() {
        if (!search_dialog) {
            search_dialog = new Fl_Window(300, 105, "Find");
            search_input = new Fl_Input(70, 10, 200, 25, "Find:");
            Fl_Button* find_button = new Fl_Button(10, 70, 90, 25, "Find");
            find_button->callback(search_callback, this);
            Fl_Button* cancel_button = new Fl_Button(200, 70, 90, 25, "Cancel");
            cancel_button->callback([](Fl_Widget*, void* v) {
                ((EditorWindow*)v)->search_dialog->hide();
            }, this);
            search_dialog->end();
        }
        search_dialog->show();
    }

    static void search_callback(Fl_Widget*, void* v) {
        EditorWindow* win = (EditorWindow*)v;
        const char* val = win->search_input->value();
        if (val && *val) {
            if (win->search_string) free(win->search_string);
            win->search_string = strdup(val);
            int pos = win->editor->insert_position();
            int found = win->textbuf->search_forward(pos, win->search_string, &pos);
            if (found) {
                win->textbuf->select(pos, pos + strlen(win->search_string));
                win->editor->insert_position(pos + strlen(win->search_string));
                win->editor->show_insert_position();
            } else {
                fl_alert("No more occurrences of '%s' found!", win->search_string);
            }
        }
        win->search_dialog->hide();
    }

    void replace_text() {
        const char* find = fl_input("Find:", "");
        if (find) {
            const char* replace = fl_input("Replace with:", "");
            if (replace) {
                int start = editor->insert_position();
                int found;
                while ((found = textbuf->search_forward(start, find, &start)) != 0) {
                    textbuf->select(start, start + strlen(find));
                    textbuf->remove_selection();
                    textbuf->insert(start, replace);
                    start += strlen(replace);
                    is_modified = true;
                }
                update_title();
            }
        }
    }

    // --- Undo/Redo Support ---
    static void changed_callback(int pos, int nInserted, int nDeleted, int nRestyled,
                                 const char* deletedText, void* v) {
        EditorWindow* win = (EditorWindow*)v;
        if (win->in_undo_redo) return;
        win->undo_stack.push_back(win->last_snapshot);
        win->redo_stack.clear();
        win->last_snapshot = win->textbuf->text();
        win->is_modified = true;
        win->update_title();
    }

    void undo_action() {
        if (undo_stack.empty()) {
            fl_alert("Nothing to undo.");
            return;
        }
        in_undo_redo = true;
        redo_stack.push_back(textbuf->text());
        std::string state = undo_stack.back();
        undo_stack.pop_back();
        textbuf->text(state.c_str());
        last_snapshot = textbuf->text();
        is_modified = true;
        update_title();
        in_undo_redo = false;
    }

    void redo_action() {
        if (redo_stack.empty()) {
            fl_alert("Nothing to redo.");
            return;
        }
        in_undo_redo = true;
        undo_stack.push_back(textbuf->text());
        std::string state = redo_stack.back();
        redo_stack.pop_back();
        textbuf->text(state.c_str());
        last_snapshot = textbuf->text();
        is_modified = true;
        update_title();
        in_undo_redo = false;
    }

    // --- Font size/style support ---
    void change_font_size() {
        const char* input = fl_input("Enter new font size:", "14");
        if (input && *input) {
            int new_size = atoi(input);
            if (new_size <= 0) {
                fl_alert("Invalid font size.");
                return;
            }
            editor->textsize(new_size);
            for (int i = 0; i < 6; i++) {
                styletable[i].size = new_size;
            }
            editor->highlight_data(stylebuf, styletable, 6, 'A', 0, 0);
            editor->redraw();
        }
    }

    void change_font_style() {
        // Ask for both font family and style.
        const char* family_input = fl_input("Enter font family (courier, helvetica, times):", "courier");
        if (!family_input) return;
        std::string family_str(family_input);
        for (auto &c : family_str) c = std::tolower(c);

        const char* style_input = fl_input("Enter font style (plain, bold, italic, bolditalic):", "plain");
        if (!style_input) return;
        std::string style_str(style_input);
        for (auto &c : style_str) c = std::tolower(c);

        int regular, bold, italic, bolditalic;
        if (family_str == "helvetica") {
            regular = FL_HELVETICA;
            bold = FL_HELVETICA_BOLD;
            italic = FL_HELVETICA_ITALIC;
            bolditalic = FL_HELVETICA_BOLD_ITALIC;
        } else if (family_str == "times") {
            regular = FL_TIMES;
            bold = FL_TIMES_BOLD;
            italic = FL_TIMES_ITALIC;
            bolditalic = FL_TIMES_BOLD_ITALIC;
        } else {
            regular = FL_COURIER;
            bold = FL_COURIER_BOLD;
            italic = FL_COURIER_ITALIC;
            bolditalic = FL_COURIER_BOLD_ITALIC;
        }

        int chosen_font;
        if (style_str == "bold") {
            chosen_font = bold;
        } else if (style_str == "italic") {
            chosen_font = italic;
        } else if (style_str == "bolditalic") {
            chosen_font = bolditalic;
        } else {
            chosen_font = regular;
        }

        // Set editor's default text font:
        editor->textfont(chosen_font);

        // Update style table variants:
        styletable[0].font = chosen_font;    // Plain text: chosen font
        styletable[1].font = bold;           // Keywords: bold
        styletable[2].font = italic;         // Comments: italic
        styletable[3].font = chosen_font;    // Strings: chosen font
        styletable[4].font = bold;           // Functions: bold
        styletable[5].font = bold;           // Numbers: bold

        editor->highlight_data(stylebuf, styletable, 6, 'A', 0, 0);
        editor->redraw();
    }

public:
    EditorWindow(int w, int h, const char* title) : Fl_Window(w, h, title) {
        textbuf = new Fl_Text_Buffer();
        stylebuf = new Fl_Text_Buffer();

        begin();
        menu = new Fl_Menu_Bar(0, 0, w, 25);
        menu->add("&File/&Open...", FL_COMMAND + 'o', menu_callback, (void*)this);
        menu->add("&File/&Save...", FL_COMMAND + 's', menu_callback, (void*)this);
        menu->add("&File/&Quit", FL_COMMAND + 'q', menu_callback, (void*)this);
        menu->add("&Edit/&Undo", FL_COMMAND + 'z', menu_callback, (void*)this);
        menu->add("&Edit/&Redo", FL_COMMAND + 'y', menu_callback, (void*)this);
        menu->add("&Edit/&Find...", FL_COMMAND + 'f', menu_callback, (void*)this);
        menu->add("&Edit/&Replace...", FL_COMMAND + 'r', menu_callback, (void*)this);
        menu->add("&Format/Font Size", FL_COMMAND + '1', menu_callback, (void*)this);
        menu->add("&Format/Font Style", FL_COMMAND + '2', menu_callback, (void*)this);
        menu->add("&Help/&About", 0, menu_callback, (void*)this);

        editor = new Fl_Text_Editor(10, 35, w - 20, h - 70);
        editor->buffer(textbuf);
        editor->highlight_data(stylebuf, styletable, sizeof(styletable)/sizeof(styletable[0]), 'A', 0, 0);
        editor->textsize(14);

        last_snapshot = textbuf->text();
        textbuf->add_modify_callback(changed_callback, this);

        end();
        resizable(editor);
    }

    ~EditorWindow() {
        delete textbuf;
        delete stylebuf;
        if (search_string) free(search_string);
        if (search_dialog) delete search_dialog;
    }
};

int main(int argc, char** argv) {
    Fl::scheme("gtk+");
    EditorWindow* win = new EditorWindow(800, 600, "FLTK Text Editor");
    win->show(argc, argv);
    return Fl::run();
}
