/*
    @Author: Jun Ivanne Dalman
    ##Supporting Members: Veejay Bermejo, Adrian Carcueva, Rodge Lorence Bumagat

    ##This project aims to provide solution for animating and creating pixel arts directly in the console_output without calling system calls-
    to emulate screen playback using the non-traditonal way to render pixel as fast as possible.

    ##This project have solved different problems when it comes to rendering in console_output:
    * Console blinking: This occurs when the console_output tries to fill the whole program with spaces to reset the screen-
    specifically by calling system(cls), thus every actions will make the console_output blink, causing the user's eye to suffer.

    * Sluggish console_output rendering: Since calling system command requires convoluted process, this project aimed to avoid that-
    and instead talked to the Window OS (windows.h) itself to clear the screen, providing a swift and unnoticeable render loading.

    * Conditional cleaning of  the screen: This was achieved by not clearing the screen when the user is interacting with tne 2D space.
    And only clearing the screen when the layout changes.

    ##This project requires some handy dependencies to work as intended:
    * C++ with STL version: Some features requires Standard Template Library
    * IDE's ANSI & ASCII support, suggestions: use Codeblocks as it supports those two very well
    * Windows OS: Huge features of this app requires windows.h header which only works for Windows
*/

//!!!IMPORTANT: Please Use Code::Blocks 20.03 as this project was developed with it

#include <iostream> //For basic I/O operations
#include <vector> //Vectors instead of the traditional fixed-size arrays to provide dynamic user experience
#include <conio.h> //To provide instant keyboard feedback
#include <windows.h> //For console_output operations
#include <fstream> //For file operations
#include <iomanip> //For structuring the file data input/output to minimize errors
#include <string> //For checking .canim file

using namespace std;

//This feature depends on C++ STL
#define underline_in "\e[4m"
#define underline_out "\e[24m"

//Initialization of the functions
void clearScrn(int attrib, HANDLE console_output, COORD pos);
void resizeVectors(vector < vector < vector < char >>> & frames, vector < vector < vector < int >>> & frame_color, int frame_count, int frame_height, int frame_width);
void batchCopyFrameOperation(vector < vector < char >> & frame_copy, vector < vector < int >> & color_copy, int i, int j, char replace_char, int replace_color);
void batchFrameOperation(vector < vector < vector < char >>> & frames, vector < vector < vector < int >>> & frame_color, int current_frame, int i, int j, char replace_char, int replace_color);
void floodFill(vector < vector < vector < char >>> & frames, vector < vector < vector < int >>> & frame_color, int current_frame, int x, int y, int targetColor, char replacementChar, int replaceColor);
bool saveAnimation(const string & filename,
                   const vector < vector < vector < char >>> & frames,
                   const vector < vector < vector < int >>> & frame_color, int & frame_count, int & frame_height, int & frame_width, int & animation_fps);
bool loadAnimation(const string & filename, vector < vector < vector < char >>> & frames, vector < vector < vector < int >>> & frame_color, int & frame_count, int & frame_height, int & frame_width, int & animation_fps);

//Mouse functions initialization
void processMouseInput(MOUSE_EVENT_RECORD mer, int& x, int& y, bool& painting, bool& erasing, char& choice, int frame_width, int frame_height, bool line_tool, bool rectangle_tool);

int main() {
    system("cls");

    //Setups
    printf("\e[?25l");
    int result = MessageBox(NULL, "Welcome to C Animator 2D!\n=====================================\n\nEnable mouse operations?\n\nWARNING: Mouse support of this program is EXPERIMENTAL\n- Toggle later by pressing '8'", "CAnimator2D - Set up", MB_YESNO);
    bool enable_mouse = false;
    if(result == IDYES)enable_mouse = true;

    //Console operations
    SetConsoleTitle(TEXT("C Animator2D v1.5.0 - Create pixel arts in your console_output"));
    HANDLE console_output = GetStdHandle(STD_OUTPUT_HANDLE);
    HANDLE console_input = GetStdHandle(STD_INPUT_HANDLE);

    //Frame initialization
    int frame_count = 10;
    int frame_height = 15;
    int frame_width = 45;
    vector < vector < vector < char >>> frames(frame_count, vector < vector < char >> (frame_height, vector < char > (frame_width, ' ')));
    vector < vector < vector < int >>> frame_color(frame_count, vector < vector < int >> (frame_height, vector < int > (frame_width, 0)));

    //State variables
    string texture = " "; // For text tool
    int current_frame = 0, current_pointer = 254, current_color = 15, current_symbol = 0, current_texture = 247, texture_color = 15, brush_copy = 0, copyBrush_color = 0;
    texture[0] = current_texture;
    int avail_symbol[] = {219,178,177,176 };
    size_t symbol_count = sizeof(avail_symbol) / sizeof(int);

    //Cursor coordinates
    int x = 0, y = 0; //For WASD and other functions like mouse

    //Onion tool
    bool onion_skin = true, lower_onion_skin = true, upper_onion_skin = true;
    int lower_onion_color = 8, upper_onion_color = 7;

    //Fill tool
    bool fill_tool = false, pick_tool = false, line_tool = false, line_start = false, line_end = false;

    //Rectangle tool
    bool rectangle_tool = false, rect_start = false, rect_end = false, filled_rectangle = false;
    int rect_x_start, rect_y_start, rect_x_end, rect_y_end;

    //Line tool
    int x_start, x_end, y_start, y_end, x_diff, y_diff, x_dir, y_dir, xy_err;

    //Playback
    int animation_fps = 8, animation_speed = 1000 / animation_fps;
    string animation_name;

    //Layout
    bool grid_toggle = false, help_toggle = false, toolbar_toggle = true, toggle_cursor = true, instruction_toggle = true, play_toggle = false;
    char top_border = 205, side_border = 186;
    string separator = "";
    for (int i = 0; i <= 30; i++) {
        char separator_start = 15;
        char separator_body = 196;
        if (i == 0 || i == 30) {
            separator += separator_start;
        } else {
            separator += separator_body;
        }
    }

    //For copy & pasting operations
    vector < vector < char >> frame_copy(frame_height, vector < char > (frame_width, ' '));
    vector < vector < int >> color_copy(frame_height, vector < int > (frame_width, 0));

    //Set up for mouse painting
    bool painting = false;
    bool erasing = false;

    //Game loop
    char choice = '@';
    while (true) {
        //!!!IMPORTANT!!!
        //Try to remove this function and replace it with 'system("cls");' to see how important this solution is
        //Disable mouse
        //Set to 0 to not clear anything
        clearScrn(0, console_output, {0,0 });
        //system("cls");

        //Mouse handler
        if(enable_mouse) {
            //The syntax may be weird but this is how Window API's works
            //First, we have to declare a varialbe which will hold the current settings of the console
            DWORD mode = 0;
            //And here we will get the CURRENT settings of the console
            GetConsoleMode(console_input, &mode);
            //And to avoid changing ANYTHING that possibly make the console crash or glitchy
            //We have to use bitwise operators
            //Here we enabled a setting to allow the console to get mouse inputs
            mode |= ENABLE_MOUSE_INPUT;
            //Here we disabled quick edit mode, this setting allows the user to highlight, copy and paste texts in console
            //The most important reason why we disable this is because it clashes with what we want, we don't want highlights as it pauses the console
            //when we click the console because it would be confusing for the users
            mode &= ~ENABLE_QUICK_EDIT_MODE;
            //And we set it here
            SetConsoleMode(console_input, mode);

            //CONSOLE SET UP FOR MODE OF INPUT
            INPUT_RECORD input_buffer_size[128];
            DWORD char_to_read;
            if (PeekConsoleInput(console_input, input_buffer_size, 128, &char_to_read) && char_to_read > 0) {
                ReadConsoleInput(console_input, input_buffer_size, 128, &char_to_read);
                for (DWORD i = 0; i < char_to_read; i++) {
                    if (input_buffer_size[i].EventType == MOUSE_EVENT) {
                        processMouseInput(input_buffer_size[i].Event.MouseEvent, x, y, painting, erasing, choice, frame_width, frame_height, line_tool, rectangle_tool);
                    }
                }
            }
        }

        //Layout
        printf("\n");

        //!TODO : Make the printing functions direct to the windows
        SetConsoleTextAttribute(console_output, 159);
        printf("\t%c %c C Animator2D                              %c\n", 179, 254, 179);
        SetConsoleTextAttribute(console_output, 240);
        printf("\t%c  ", 186);
        printf("Save Anima");
        SetConsoleTextAttribute(console_output, 241);
        printf("%st%s",underline_in,underline_out);
        SetConsoleTextAttribute(console_output, 240);
        printf("ion");
        printf("  %c  ", 179);
        printf("Op");
        SetConsoleTextAttribute(console_output, 241);
        printf("%se%s",underline_in,underline_out);
        SetConsoleTextAttribute(console_output, 240);
        printf("n");
        printf("  %c  ", 179);
        printf("Toggle ");
        SetConsoleTextAttribute(console_output, 241);
        printf("%sH%s",underline_in,underline_out);
        SetConsoleTextAttribute(console_output, 240);
        printf("elp  ");
        printf("  %c\n", 186);
        SetConsoleTextAttribute(console_output, 15);

        string top_x_border = "";
        string bottom_x_border = "";
        for (int i = 0; i < frame_width + 1; i++) {
            char top_left = 201;
            i == 0 ? top_x_border += top_left : top_x_border += top_border;
            if (i == frame_width) {
                char top_right = 187;
                top_x_border += top_right;
            }
        }
        for (int i = 0; i < frame_width + 1; i++) {
            char bottom_left = 200;
            i == 0 ? bottom_x_border += bottom_left : bottom_x_border += top_border;
            if (i == frame_width) {
                char bottom_right = 188;
                bottom_x_border += bottom_right;
            }
        }
        /*
        ### Purpose:
        This code snippet is responsible for rendering a frame of CAnimator2D. It iterates through each row and column of the frame to display various elements including borders, cursor, drawing tools (like rectangle and line), grid, frame contents, and onion skinning effects.

        ### Steps:
        1. Outer Loop (Rows):
           - Iterates through each row (`i`) of the frame.
        2. Space Layout:
           - Constructs a string (`space_layout`) containing spaces to correctly layout the content horizontally.
        3. Top Border:
           - Displays the top border of the frame if `i` is 0. Optionally, renders a toolbar title if `instruction_toggle` and `toolbar_toggle` are true.
        4. Inner Loop (Columns):
           - Iterates through each column (`j`) of the frame.
        5. Cursor Rendering:
           - Checks if the current position (`i`, `j`) matches the cursor position (`y`, `x`) and renders the cursor if `toggle_cursor` is true.
        6. Rectangle Tool:
           - Renders rectangles based on user input (`rect_start`, `rectangle_tool`). Determines the boundaries and fills or outlines the rectangle as per settings.
        7. Line Tool:
           - Implements Bresenham's Line Algorithm to draw lines between two points (`line_start`, `line_tool`). Renders each pixel of the line between `x_start`, `y_start` and `x`, `y`.
        8. Grid Toggle:
           - Optionally displays a grid (`underline_in`) over the frame contents.
        9. Frame Contents:
           - Displays characters stored in `frames[current_frame][i][j]`, applying color attributes (`frame_color`) as needed.
        10. Onion Skin:
            - Optionally displays faded versions of adjacent frames (`lower_onion_color`, `upper_onion_color`) if `onion_skin` is enabled.
        */

        for (int i = 0; i < frame_height; i++) {
            string space_layout = "";
            for(int i = 0; i < 45 - frame_width; i++) {
                space_layout += " ";
            }
            cout << "\t";

            //Layout
            if (i == 0) {
                cout << top_x_border << " ";
                if(instruction_toggle && toolbar_toggle) {
                    cout << space_layout;
                    SetConsoleTextAttribute(console_output, 63);
                    printf("<========== Toolbar ==========>");
                    SetConsoleTextAttribute(console_output, 15);
                }
                printf("\n\t");
            }
            for (int j = 0; j < frame_width; j++) {
                if (j == 0) cout << side_border;

                //Cursor rendering
                if (i == y && j == x && toggle_cursor) {
                    SetConsoleTextAttribute(console_output, 15);
                    printf("%c", current_pointer);
                } else {
                    bool printed = false;
                    if (rectangle_tool && rect_start) {
                        int x_min = min(rect_x_start, x);
                        int x_max = max(rect_x_start, x);
                        int y_min = min(rect_y_start, y);
                        int y_max = max(rect_y_start, y);

                        if (j >= x_min && j <= x_max && i >= y_min && i <= y_max) {
                            if (!filled_rectangle) {
                                if (j == x_min || j == x_max || i == y_min || i == y_max) {
                                    SetConsoleTextAttribute(console_output, current_color);
                                    printf("%c", avail_symbol[current_symbol]);
                                    SetConsoleTextAttribute(console_output, 15);
                                    printed = true;
                                }
                            } else {
                                SetConsoleTextAttribute(console_output, current_color);
                                printf("%c", avail_symbol[current_symbol]);
                                SetConsoleTextAttribute(console_output, 15);
                                printed = true;
                            }
                        }
                    }
                    //Line tool using Bresenhams Algorithm : https://en.wikipedia.org/wiki/Bresenham%27s_line_algorithm
                    if (line_tool && line_start) {
                        int temp_x = x_start, temp_y = y_start;
                        int temp_x_diff = abs(x - x_start);
                        int temp_y_diff = abs(y - y_start);
                        int temp_x_dir = (x_start < x) ? 1 : -1;
                        int temp_y_dir = (y_start < y) ? 1 : -1;
                        int temp_xy_err = temp_x_diff - temp_y_diff;

                        while (true) {
                            if (temp_x == j && temp_y == i) {
                                SetConsoleTextAttribute(console_output, current_color);
                                printf("%c", avail_symbol[current_symbol]);
                                SetConsoleTextAttribute(console_output, 15);
                                printed = true;
                                break;
                            }
                            if (temp_x == x && temp_y == y) break;
                            int err_temp = temp_xy_err * 2;
                            if (err_temp > -temp_y_diff) {
                                temp_xy_err -= temp_y_diff;
                                temp_x += temp_x_dir;
                            }
                            if (err_temp < temp_x_diff) {
                                temp_xy_err += temp_x_diff;
                                temp_y += temp_y_dir;
                            }
                        }
                    }

                    //Prints the contents of the vector
                    if (!printed) {
                        if (grid_toggle) printf("%s", underline_in);
                        if (frames[current_frame][i][j] != ' ') {
                            SetConsoleTextAttribute(console_output, frame_color[current_frame][i][j]);
                            printf("%c", frames[current_frame][i][j]);
                            SetConsoleTextAttribute(console_output, 15);
                        } else {
                            if (onion_skin && ((current_frame > 0 && frames[current_frame - 1][i][j] != ' ') || (current_frame < frame_count - 1 && frames[current_frame + 1][i][j] != ' '))) {
                                if (current_frame > 0 && frames[current_frame - 1][i][j] != ' ') {
                                    SetConsoleTextAttribute(console_output, lower_onion_color);
                                    lower_onion_skin ? printf("%c", frames[current_frame - 1][i][j]) : printf("%c", frames[current_frame][i][j]);
                                } else if (current_frame < frame_count - 1 && frames[current_frame + 1][i][j] != ' ') {
                                    SetConsoleTextAttribute(console_output, upper_onion_color);
                                    upper_onion_skin ? printf("%c", frames[current_frame + 1][i][j]) : printf("%c", frames[current_frame][i][j]);
                                }
                                SetConsoleTextAttribute(console_output, 15);
                            } else {
                                printf("%c", frames[current_frame][i][j]);
                            }
                        }
                    }
                }
                if (grid_toggle) printf("%s", underline_out);
                if (j == frame_width - 1) cout << side_border;

                //Layout to visually enhance the program and make the UX better
                if (toolbar_toggle) {
                    SetConsoleTextAttribute(console_output, 7);
                    if (i == 0 && j == frame_width - 1) {
                        cout << space_layout;
                        (instruction_toggle) ? printf(" %c %s", 17, "|P| & |O| to put and remove paint"): line_tool ? printf(" %c ON", 17) : printf(" %c OFF", 17);
                    }
                    if (i == 1 && j == frame_width - 1) {
                        cout << space_layout;
                        (instruction_toggle) ? printf(" %c %s", 17, "|U| to copy a color"): fill_tool ? printf(" %c ON", 17) : printf(" %c OFF", 17);
                    }
                    if(i == 2 && j == frame_width - 1)if(instruction_toggle)cout << space_layout << " " << separator;
                    if (i == 3 && j == frame_width - 1) {
                        cout << space_layout;
                        if (instruction_toggle) line_tool ? printf(" %c ON", 17) : printf(" %c OFF", 17);
                        if (instruction_toggle) printf(" -> %s", "|I| Line tool");
                        (instruction_toggle) ?
                        rectangle_tool ? printf("\t%c ON", 17) : printf("\t%c OFF", 17):
                        rectangle_tool ? printf(" %c ON", 17) : printf(" %c OFF", 17);
                        if (instruction_toggle) printf(" -> %s", "|Z| Rectangle tool");
                    }
                    if (i == 4 && j == frame_width - 1) {
                        cout << space_layout;
                        if (instruction_toggle) fill_tool ? printf(" %c ON", 17) : printf(" %c OFF", 17);
                        if (instruction_toggle) printf(" -> %s", "|F| Fill-tool");
                        (instruction_toggle) ?
                        filled_rectangle ? printf("\t%c ON", 17) : printf("\t%c OFF", 17):
                        filled_rectangle ? printf(" %c ON", 17) : printf(" %c OFF", 17);
                        if (instruction_toggle) printf(" -> %s", "|X| Fill Rectangle");
                    }
                    if(i == 5 && j == frame_width - 1)if(instruction_toggle)cout << space_layout << " " << separator;
                    if (i == 6 && j == frame_width - 1) {
                        cout << space_layout;
                        SetConsoleTextAttribute(console_output, 15);
                        printf(" %c %c", 17, avail_symbol[current_symbol]);
                        SetConsoleTextAttribute(console_output, 7);
                        if (instruction_toggle) printf(" -> |-| & |=| Change paint texture");
                    }
                    if (i == 7 && j == frame_width - 1) {
                        cout << space_layout;
                        printf(" %c ", 17);
                        SetConsoleTextAttribute(console_output, current_color);
                        printf("%c", avail_symbol[current_symbol]);
                        SetConsoleTextAttribute(console_output, 7);
                        if (instruction_toggle) printf(" -> %s", "|[| & |]| Change paint color");
                    }
                    if(i == 8 && j == frame_width - 1)if(instruction_toggle)cout << space_layout << " " << separator;
                    if (i == 9 && j == frame_width - 1) {
                        cout << space_layout;
                        onion_skin ? printf(" %c ON", 17) : printf(" %c OFF", 17);
                        if (instruction_toggle) printf(" -> %s", "|M| Toggle onion skin");
                    }
                    if (i == 10 && j == frame_width - 1) {
                        cout << space_layout;
                        printf(" %c %c", 17, current_texture);
                        if (instruction_toggle) printf(" -> |R| to put a text symbol");
                    }
                    if (i == 11 && j == frame_width - 1) {
                        cout << space_layout;
                        printf(" %c ", 17);
                        SetConsoleTextAttribute(console_output, texture_color);
                        printf("%c ", current_texture);
                        SetConsoleTextAttribute(console_output, 7);
                        if (instruction_toggle) printf("-> %s ", "|{| & |}| to change color of unique symbol");
                    }
                    if(i == 12 && j == frame_width - 1)if(instruction_toggle)cout << space_layout << " " << separator;
                    if (i == 13 && j == frame_width - 1) {
                        cout << space_layout;
                        printf(" %c %02d", 17, current_frame + 1);
                        if (instruction_toggle) printf(" -> |K| & |L| Change frame");
                    }
                    if (i == 14 && j == frame_width - 1) {
                        cout << space_layout;
                        printf(" %c %02d", 17, frame_count);
                        if (instruction_toggle) printf(" -> |N| & |B| Create / remove frame");
                    }
                }
            }
            if (i == frame_height - 1) cout << "\n" << "\t" << bottom_x_border;
            printf("\n");
        }
        printf("\t%c %c %02d | %c %02d | %c %02d / %02d | %c %02d FPS %c", 186, 260, x, 260, y, 260, current_frame + 1, frame_count, 260, animation_fps, 186);
        printf("\n\tFile name: ");
        (animation_name.size() == 0) ? cout << "Not saved": cout << animation_name;
        cout << "\n";

        //Playback
        if (play_toggle) {
            animation_speed = 1000 / animation_fps;
            Sleep(animation_speed);
            if (current_frame + 1 == frame_count) {
                Beep(700, 200);
                current_frame = 0;
                toolbar_toggle = true;
                toggle_cursor = true;
                play_toggle = false;
            } else {
                current_frame++;
            }
        }

        //Layout to help user what are the other available commands
        if (help_toggle) {
            printf("\n");
            SetConsoleTextAttribute(console_output, 248);
            printf("\t%c============== Program Feature ==============%c",186, 186);
            SetConsoleTextAttribute(console_output, 15);
            printf("\n\n");
            SetConsoleTextAttribute(console_output, 159);
            printf("\t %c Resizing Canvas %c ",260, 260);
            SetConsoleTextAttribute(console_output, 15);
            printf("\n\t|1| & |2| Resize canvas width\t\t|3| & |4| Resize canvas height\n\n");

            SetConsoleTextAttribute(console_output, 159);
            printf("\t %c Canvas Paint Operation %c ",260, 260);
            SetConsoleTextAttribute(console_output, 15);
            printf("\n\t|C| & |V| Copy & paste canvas\t\t|0| Clear the canvas\n\n");

            SetConsoleTextAttribute(console_output, 159);
            printf("\t %c Toggle Layouts %c ",260, 260);
            SetConsoleTextAttribute(console_output, 15);
            printf("\n\t|G| Toggle lines\t\t\t|Y| & |*| Toggle toolbar or cursor\n");
            printf("\t|?| Toggle instructions\n\n");

            SetConsoleTextAttribute(console_output, 159);
            printf("\t %c Texture Brush Options %c ",260, 260);
            SetConsoleTextAttribute(console_output, 15);
            printf("\n\t|<| & |>| Copy and paste textures\t|J| See available texture brush\n");
            printf("\t|5| Choose texture\t\t\t|6| & |R| Text tool\n\n");

            SetConsoleTextAttribute(console_output, 159);
            printf("\t %c Save Shortcuts %c ",260, 260);
            SetConsoleTextAttribute(console_output, 15);
            printf("\n\t|\\| Quickly save file\t\t\t|T|,|E|,|Q| Save, open & create new file\n");
            printf("\t|!| to delete a file\n\n");

            SetConsoleTextAttribute(console_output, 159);
            printf("\t %c Onion Skin Settings %c ",260, 260);
            SetConsoleTextAttribute(console_output, 15);
            SetConsoleTextAttribute(console_output, upper_onion_color);
            printf("\n\t %c", 219);
            SetConsoleTextAttribute(console_output, 15);
            printf(" %s", "|;| & |'| Change upper onion color");

            SetConsoleTextAttribute(console_output, lower_onion_color);
            printf("\t %c", 219);
            SetConsoleTextAttribute(console_output, 15);
            printf(" %s", "|,| & |.| Change lower onion color\n\n");

            SetConsoleTextAttribute(console_output, 159);
            printf("\t %c Play Shortcuts %c ",260, 260);
            SetConsoleTextAttribute(console_output, 15);
            printf("\n\t|Space| & |#| Play & Change animation speed\n");
            printf("\tTip: Press 'Space' only once and wait for the animation to stop\n\n");

            SetConsoleTextAttribute(console_output, 159);
            printf("\t %c Mouse Operations %c ",260, 260);
            SetConsoleTextAttribute(console_output, 15);
            printf("\n");
            SetConsoleTextAttribute(console_output, 79);
            printf("\tWARNING: EXPERIMENTAL, ENABLE AT YOUR OWN RISK");
            SetConsoleTextAttribute(console_output, 15);
            printf("\n\t|8| to enable mouse operations");
            printf("\n\t|Left| & |Right| Hold or press to put & erase paint\n\n");
        }

        //Actions
        if(enable_mouse) {
            if(kbhit()) {
                if (!play_toggle) choice = getch();
            }
        } else {
            if (!play_toggle) choice = getch();
        }
        if (choice == 'W' || choice == 'w') y > 0 ? y-- : y = frame_height - 1;
        if (choice == 'S' || choice == 's') y < frame_height - 1 ? y++ : y = 0;
        if (choice == 'A' || choice == 'a') x > 0 ? x-- : x = frame_width - 1;
        if (choice == 'D' || choice == 'd') x < frame_width - 1 ? x++ : x = 0;
        if (choice == 'P' || choice == 'p') {

            //Flood fill algorithm : https://en.wikipedia.org/wiki/Flood_fill
            if (fill_tool) {
                int target_color = frame_color[current_frame][y][x];
                floodFill(frames, frame_color, current_frame, x, y, target_color, avail_symbol[current_symbol], current_color);
                fill_tool = false;
            }
            if (rectangle_tool) {
                if (!rect_start) {
                    rect_start = true;
                    rect_x_start = x;
                    rect_y_start = y;
                } else {
                    rect_end = true;
                    rect_x_end = x;
                    rect_y_end = y;
                    rect_start = false;
                    rectangle_tool = false;
                }
            } else if (line_tool) {
                if (!line_start) {
                    line_start = true;
                    x_start = x;
                    y_start = y;
                } else {
                    line_tool = false;
                    line_start = false;
                    line_end = true;
                    x_end = x;
                    y_end = y;
                    x_diff = abs(x_end - x_start);
                    y_diff = abs(y_end - y_start);
                    x_dir = (x_start < x_end) ? 1 : -1;
                    y_dir = (y_start < y_end) ? 1 : -1;
                    xy_err = x_diff - y_diff;
                }
            } else {
                frame_color[current_frame][y][x] = current_color;
                frames[current_frame][y][x] = avail_symbol[current_symbol];
            }
            if (rect_end) {
                int x_min = min(rect_x_start, rect_x_end);
                int x_max = max(rect_x_start, rect_x_end);
                int y_min = min(rect_y_start, rect_y_end);
                int y_max = max(rect_y_start, rect_y_end);

                for (int i = 0; i <= frame_height; i++) {
                    for (int j = 0; j <= frame_width; j++) {
                        if (j >= x_min && j <= x_max && i >= y_min && i <= y_max) {
                            if (!filled_rectangle) {
                                if (j == x_min || j == x_max || i == y_min || i == y_max) {
                                    frame_color[current_frame][i][j] = current_color;
                                    frames[current_frame][i][j] = avail_symbol[current_symbol];
                                }
                            } else {
                                frame_color[current_frame][i][j] = current_color;
                                frames[current_frame][i][j] = avail_symbol[current_symbol];
                            }
                            continue;
                        }
                    }
                }
                rect_end = false;
            }
            if (line_end) {
                line_end = false;
                while (true) {
                    if (x_start >= 0 && x_start < frame_width && y_start >= 0 && y_start < frame_height) {
                        frame_color[current_frame][y_start][x_start] = current_color;
                        frames[current_frame][y_start][x_start] = avail_symbol[current_symbol];
                    }
                    if (x_start == x_end && y_start == y_end) break;
                    int err_temp = xy_err * 2;
                    if (err_temp > -y_diff) {
                        xy_err -= y_diff;
                        x_start += x_dir;
                    }
                    if (err_temp < x_diff) {
                        xy_err += x_diff;
                        y_start += y_dir;
                    }
                }
            }
        }
        if (choice == '=')
            if (current_symbol > 0) current_symbol--;
        if (choice == '-')
            if (current_symbol < symbol_count - 1) current_symbol++;
        if (choice == 'L' || choice == 'l') current_frame < frame_count - 1 ? current_frame++ : current_frame = 0;
        if (choice == 'K' || choice == 'k') current_frame > 0 ? current_frame-- : current_frame = frame_count - 1;
        if (choice == 'O' || choice == 'o') {
            if (fill_tool) {
                int target_color = frame_color[current_frame][y][x];
                floodFill(frames, frame_color, current_frame, x, y, target_color, ' ', 0);
                fill_tool = false;
            }
            if (rectangle_tool) {
                if (!rect_start) {
                    rect_start = true;
                    rect_x_start = x;
                    rect_y_start = y;
                } else {
                    rect_end = true;
                    rect_x_end = x;
                    rect_y_end = y;
                    rect_start = false;
                    rectangle_tool = false;
                }
            } else if (line_tool) {
                if (!line_start) {
                    line_start = true;
                    x_start = x;
                    y_start = y;
                } else {
                    line_tool = false;
                    line_start = false;
                    line_end = true;
                    x_end = x;
                    y_end = y;
                    x_diff = abs(x_end - x_start);
                    y_diff = abs(y_end - y_start);
                    x_dir = (x_start < x_end) ? 1 : -1;
                    y_dir = (y_start < y_end) ? 1 : -1;
                    xy_err = x_diff - y_diff;
                }
            } else {
                frame_color[current_frame][y][x] = 0;
                frames[current_frame][y][x] = ' ';
            }
            if (rect_end) {
                int x_min = min(rect_x_start, rect_x_end);
                int x_max = max(rect_x_start, rect_x_end);
                int y_min = min(rect_y_start, rect_y_end);
                int y_max = max(rect_y_start, rect_y_end);

                for (int i = 0; i <= frame_height; i++) {
                    for (int j = 0; j <= frame_width; j++) {
                        if (j >= x_min && j <= x_max && i >= y_min && i <= y_max) {
                            if (!filled_rectangle) {
                                if (j == x_min || j == x_max || i == y_min || i == y_max) {
                                    frame_color[current_frame][i][j] = 0;
                                    frames[current_frame][i][j] = ' ';
                                }
                            } else {
                                frame_color[current_frame][i][j] = 0;
                                frames[current_frame][i][j] = ' ';
                            }
                            continue;
                        }
                    }
                }
                rect_end = false;
            }
            if (line_end) {
                line_end = false;
                while (true) {
                    if (x_start >= 0 && x_start < frame_width && y_start >= 0 && y_start < frame_height) {
                        frame_color[current_frame][y_start][x_start] = 0;
                        frames[current_frame][y_start][x_start] = ' ';
                    }
                    if (x_start == x_end && y_start == y_end) break;
                    int err_temp = xy_err * 2;
                    if (err_temp > -y_diff) {
                        xy_err -= y_diff;
                        x_start += x_dir;
                    }
                    if (err_temp < x_diff) {
                        xy_err += x_diff;
                        y_start += y_dir;
                    }
                }
            }
        }
        if (choice == '[')
            if (current_color > 0) current_color--;
        if (choice == ']')
            if (current_color < 15) current_color++;
        if (choice == '{')
            if (texture_color > 0) texture_color--;
        if (choice == '}')
            if (texture_color < 15) texture_color++;
        if (choice == 'M' || choice == 'm') onion_skin = !onion_skin;
        if (choice == ',')
            if (lower_onion_color > 0) lower_onion_color--;
        if (choice == '.')
            if (lower_onion_color < 15) lower_onion_color++;
        if (choice == ';')
            if (upper_onion_color > 0) upper_onion_color--;
        if (choice == '\'')
            if (upper_onion_color < 15) upper_onion_color++;
        if (choice == 'B' || choice == 'b') {
            if (frame_count > 1) {
                frames.erase(frames.begin() + current_frame);
                frame_color.erase(frame_color.begin() + current_frame);
                frame_count--;
                if (current_frame >= frame_count) current_frame = frame_count - 1;
            }
        }
        if (choice == 'N' || choice == 'n') {
            frames.insert(frames.begin() + current_frame + 1, vector < vector < char >> (frame_height, vector < char > (frame_width, ' ')));
            frame_color.insert(frame_color.begin() + current_frame + 1, vector < vector < int >> (frame_height, vector < int > (frame_width, 0)));
            frame_count++;
            current_frame++;
        }
        if (choice == 'I' || choice == 'i') {
            Beep(500, 100);
            line_tool = !line_tool;
            line_start = false;
            fill_tool = false;
            rectangle_tool = false;
        }
        if (choice == 'F' || choice == 'f') {
            Beep(500, 100);
            line_tool = false;
            rectangle_tool = false;
            fill_tool = !fill_tool;
        }
        if (choice == '1' || choice == '2' || choice == '3' || choice == '4') {
            if (choice == '1') frame_width > 1 ? frame_width-- : frame_width = 1;
            if (choice == '2') frame_width++;
            if (choice == '3') frame_height > 1 ? frame_height-- : frame_height = 1;
            if (choice == '4') frame_height++;
            for (auto& frame_col : frame_color) {
                frame_col.resize(frame_height);
                for (auto& row_col : frame_col) {
                    row_col.resize(frame_width, 0);
                }
            }
            for (auto& frame : frames) {
                frame.resize(frame_height);
                for (auto& row : frame) {
                    row.resize(frame_width, ' ');
                }
            }
            frame_copy.resize(frame_height);
            for (auto & row: frame_copy) {
                row.resize(frame_width, ' ');
            }
            color_copy.resize(frame_height);
            for (auto & row: color_copy) {
                row.resize(frame_width, 0);
            }
            clearScrn(100000, console_output, {0, 0 });
        }
        if (choice == 'T' || choice == 't') {
            Beep(500, 100);
            printf("\n");
            string command = "dir /a-d | findstr /i ";
            system((command + "\".txt\"").c_str());
            cout << "\n\tExample Input: animation.canim -> Enter: 'x' to cancel" << endl;
            cout << "\tSave animation as: ";
            string name;
            cin >> name;
            if (name == "X" || name == "x") {
                clearScrn(500000, console_output, {0, 0 });
                choice = '@';
                continue;
            }
            bool isSuccess = saveAnimation(name, frames, frame_color, frame_count, frame_height, frame_width, animation_fps);
            if (isSuccess) animation_name = name;
            x = 0;
            y = 0;
            clearScrn(500000, console_output, {0, 0 });
        }
        if (choice == 'E' || choice == 'e') {
            Beep(500, 100);
            printf("\n");
            string command = "dir /a-d | findstr /i ";
            system((command + "\".canim\"").c_str());
            cout << "\n\tExample Input: animation.canim -> Enter: 'x' to cancel" << endl;
            cout << "\tLoad animation: ";
            string name;
            cin >> name;
            if (name == "X" || name == "x") {
                clearScrn(500000, console_output, {0, 0 });
                choice = '@';
                continue;
            }
            bool isSuccess = loadAnimation(name, frames, frame_color, frame_count, frame_height, frame_width, animation_fps);
            if (isSuccess) animation_name = name;
            x = 0;
            y = 0;
            clearScrn(500000, console_output, {0, 0 });
        }
        if (choice == 'G' || choice == 'g') grid_toggle = !grid_toggle;
        if (choice == 'C' || choice == 'c') {
            Beep(500, 100);
            for (int i = 0; i < frame_height; i++)
                for (int j = 0; j < frame_width; j++) batchCopyFrameOperation(frame_copy, color_copy, i, j, ' ', 0);
            for (int i = 0; i < frame_height; i++)
                for (int j = 0; j < frame_width; j++)
                    if (frames[current_frame][i][j] != ' ') batchCopyFrameOperation(frame_copy, color_copy, i, j, frames[current_frame][i][j], frame_color[current_frame][i][j]);
        };
        if (choice == 'V' || choice == 'v') {
            Beep(500, 100);
            for (int i = 0; i < frame_height; i++)
                for (int j = 0; j < frame_width; j++)
                    if (frame_copy[i][j] != ' ') batchFrameOperation(frames, frame_color, current_frame, i, j, frame_copy[i][j], color_copy[i][j]);
        };
        if (choice == '0') {
            Beep(500, 100);
            printf("\n\tClear canvas? Y/N: ");
            char confirm = getch();
            if(confirm == 'Y' || confirm == 'y') {
                for (int i = 0; i < frame_height; i++)
                    for (int j = 0; j < frame_width; j++) batchFrameOperation(frames, frame_color, current_frame, i, j, ' ', 0);
            }
            clearScrn(100000, console_output, {0,0 });
        }
        if (choice == 'U' || choice == 'u') current_color = frame_color[current_frame][y][x];
        if (choice == 'H' || choice == 'h') {
            clearScrn(500000, console_output, {0, 0 });
            help_toggle = !help_toggle;
        }
        if (choice == 'Y' || choice == 'y') {
            clearScrn(500000, console_output, {0, 0 });
            toolbar_toggle = !toolbar_toggle;
        }
        if (choice == '\\') {
            Beep(500, 100);
            if (animation_name.size() != 0) {
                bool isSuccess = saveAnimation(animation_name, frames, frame_color, frame_count, frame_height, frame_width, animation_fps);
                if (isSuccess) {
                    printf("\n\t%c %c Successfully saved %c %c\n\n", 186, 260, 260, 186);
                    system("pause");
                };
                clearScrn(500000, console_output, {0, 0 });
            }
        }
        if (choice == 'J' || choice == 'j') {
            cout << endl << endl;
            for (int i = 0; i < 255; i++) {
                printf("%d|%c\t", i, i);
                if (i % 5 == 0) printf("\n");
            }
            cout << "\n\nType the number of the texture: ";
            texture = " ";
            cin >> current_texture;
            texture[0] = current_texture;
            cout << endl;
            clearScrn(500000, console_output, {0, 0 });
        }
        if ((choice == 'R' || choice == 'r') && texture_color != frame_color[current_frame][y][x]) {
            for (int i = 0; i < texture.size(); i++) {
                if (x + i > frame_width) break;
                if (texture[i] != ' ') {
                    frames[current_frame][y][x + i] = texture[i];
                    frame_color[current_frame][y][x + i] = (frame_color[current_frame][y][x + i] * 16) + texture_color;
                }
            }
        }
        if (choice == '5') {
            Beep(500, 100);
            cout << "\n\tType the ASCII number of the texture: ";
            texture = " ";
            cin >> current_texture;
            texture[0] = current_texture;
            cout << endl;
            clearScrn(500000, console_output, {0, 0 });
        }
        if (choice == '6') {
            Beep(500, 100);
            cout << "\n\tType the text you want to place: ";
            getline(cin, texture);
            cout << endl;
            clearScrn(500000, console_output, {0, 0 });
        }
        if (choice == '<' && frame_color[current_frame][y][x] != 0) {
            copyBrush_color = frame_color[current_frame][y][x];
            brush_copy = frames[current_frame][y][x];
        }
        if (choice == '>' && brush_copy != 0) {
            frames[current_frame][y][x] = brush_copy;
            frame_color[current_frame][y][x] = copyBrush_color;
        }
        if (choice == '*') toggle_cursor = !toggle_cursor;
        if (choice == 'Z' || choice == 'z') {
            Beep(500, 100);
            clearScrn(10000, console_output, {0, 0 });
            rectangle_tool = !rectangle_tool;
            rect_start = false;
            line_tool = false;
            fill_tool = false;
        }
        if (choice == 'X' || choice == 'x') {
            Beep(500, 100);
            clearScrn(10000, console_output, {0, 0 });
            filled_rectangle = !filled_rectangle;
        }
        if (choice == '?') {
            clearScrn(500000, console_output, {0, 0 });
            instruction_toggle = !instruction_toggle;
        }
        if (choice == ' ') {
            Beep(500, 100);
            if (!play_toggle) clearScrn(100000, console_output, {0, 0 });
            toggle_cursor = false;
            line_tool = false;
            rectangle_tool = false;
            fill_tool = false;
            toolbar_toggle = false;
            help_toggle = false;
            onion_skin = false;
            play_toggle = true;
        }
        if (choice == '#') {
            printf("\n\tSample input: 15");
            printf("\n\tChange animation speed: ");
            cin >> animation_fps;
            cout << endl;
            clearScrn(100000, console_output, {0, 0 });
        }
        if (choice == 'Q' || choice == 'q') {
            Beep(500, 100);
            printf("\n");
            string command = "dir /a-d | findstr /i ";
            system((command + "\".canim\"").c_str());
            string name;
            int temp_width;
            int temp_height;
            int temp_count;
            int temp_fps;
            cout << "\n\tExample Input: animation.canim -> Enter: 'x' to cancel" << endl;
            cout << "\tCreate new animation: ";
            cin >> name;
            if (name == "X" || name == "x") {
                clearScrn(500000, console_output, {0, 0 });
                choice = '@';
                continue;
            }
            cout << "\tFrame width e.g 45: ";
            cin >> temp_width;
            cout << "\tFrame height e.g 15: ";
            cin >> temp_height;
            cout << "\tFrames count e.g 10: ";
            cin >> temp_count;
            cout << "\tFrame per second e.g 8: ";
            cin >> temp_fps;
            frames.clear();
            frame_color.clear();
            x = 0;
            y = 0;
            resizeVectors(frames, frame_color, temp_count, temp_height, temp_width);
            bool isSuccess = saveAnimation(name, frames, frame_color, temp_count, temp_height, temp_width, temp_fps);
            isSuccess = loadAnimation(name, frames, frame_color, frame_count, frame_height, frame_width, animation_fps);
            if (isSuccess) animation_name = name;
            clearScrn(500000, console_output, {0, 0 });
        }
        if(choice == '!') {
            Beep(500, 100);
            printf("\n");
            string command = "dir /a-d | findstr /i ";
            system((command + "\".canim\"").c_str());
            cout << "\n\tExample Input: animation.canim -> Enter: 'x' to cancel" << endl;
            cout << "\tDelete animation: ";
            string name;
            getline(cin, name);
            if (name == "X" || name == "x") {
                clearScrn(500000, console_output, {0, 0 });
                choice = '@';
                continue;
            }
            if (name.find(".canim") == string::npos) {
                cout << "\n\t" << name << " is not a .canim file\n\n";
                system("pause");
                clearScrn(100000, console_output, {0,0 });
                choice = '@';
                continue;
            }
            if(remove(name.c_str()) == 0) {
                cout << "\n\t" << name << " is successfully removed\n\n";
                system("pause");
            } else {
                cout << "\n\t" << name << " cannot be found/error\n\n";
                system("pause");
            }
            clearScrn(100000, console_output, {0,0 });
        }
        if(choice == '8') {
            Beep(500, 100);
            enable_mouse = !enable_mouse;
            help_toggle = false;
            clearScrn(100000, console_output, {0,0 });
        }
        choice = '@';
    }
    return 0;
}

void clearScrn(int attrib, HANDLE console_output, COORD pos) {
    DWORD written;
    FillConsoleOutputCharacter(console_output, ' ', attrib, pos, & written);
    FillConsoleOutputAttribute(console_output, 15, attrib, pos, & written);
    SetConsoleCursorPosition(console_output, pos);
}

void floodFill(vector < vector < vector < char >>> & frames, vector < vector < vector < int >>> & frame_color, int current_frame, int x, int y, int targetColor, char replacementChar, int replaceColor) {
    if (x < 0 || x >= frames[0][0].size() || y < 0 || y >= frames[0].size() || frame_color[current_frame][y][x] != targetColor || frame_color[current_frame][y][x] == replaceColor) return;
    frames[current_frame][y][x] = replacementChar;
    frame_color[current_frame][y][x] = replaceColor;
    floodFill(frames, frame_color, current_frame, x + 1, y, targetColor, replacementChar, replaceColor);
    floodFill(frames, frame_color, current_frame, x - 1, y, targetColor, replacementChar, replaceColor);
    floodFill(frames, frame_color, current_frame, x, y + 1, targetColor, replacementChar, replaceColor);
    floodFill(frames, frame_color, current_frame, x, y - 1, targetColor, replacementChar, replaceColor);
}

bool saveAnimation(const string & filename,
                   const vector < vector < vector < char >>> & frames,
                   const vector < vector < vector < int >>> & frame_color, int & frame_count, int & frame_height, int & frame_width, int & animation_fps) {
    ofstream outFile(filename);
    if (!outFile) {
        cout << "\tCannot create file" << endl;
        system("pause");
        return false;
    }
    outFile << frame_count << " " << frame_height << " " << frame_width << " " << animation_fps << endl;
    for (int i = 0; i < frame_count; i++) {
        for (int j = 0; j < frame_height; j++) {
            for (int k = 0; k < frame_width; k++) {
                outFile << setw(2) << setfill('0') << static_cast < int > (frames[i][j][k]) << " ";
                outFile << setw(2) << setfill('0') << frame_color[i][j][k] << " ";
            }
            outFile << endl;
        }
    }
    outFile.close();
    return true;
}

bool loadAnimation(const string & filename, vector < vector < vector < char >>> & frames, vector < vector < vector < int >>> & frame_color, int & frame_count, int & frame_height, int & frame_width, int & animation_fps) {
    ifstream inFile(filename);
    if (!inFile) {
        cout << "\tCannot read file" << endl;
        system("pause");
        return false;
    }
    inFile >> frame_count >> frame_height >> frame_width >> animation_fps;
    resizeVectors(frames, frame_color, frame_count, frame_height, frame_width);
    for (int i = 0; i < frame_count; i++) {
        for (int j = 0; j < frame_height; j++) {
            for (int k = 0; k < frame_width; k++) {
                int frameValue, colorValue;
                inFile >> frameValue;
                inFile >> colorValue;
                frames[i][j][k] = static_cast < char > (frameValue);
                frame_color[i][j][k] = colorValue;
            }
        }
    }
    inFile.close();
    return true;
}

void batchFrameOperation(vector < vector < vector < char >>> & frames, vector < vector < vector < int >>> & frame_color, int current_frame, int i, int j, char replace_char, int replace_color) {
    frames[current_frame][i][j] = replace_char;
    frame_color[current_frame][i][j] = replace_color;
}

void batchCopyFrameOperation(vector < vector < char >> & frame_copy, vector < vector < int >> & color_copy, int i, int j, char replace_char, int replace_color) {
    frame_copy[i][j] = replace_char;
    color_copy[i][j] = replace_color;
}

void resizeVectors(vector < vector < vector < char >>> & frames, vector < vector < vector < int >>> & frame_color, int frame_count, int frame_height, int frame_width) {
    frames.resize(frame_count);
    for (auto & frame: frames) {
        frame.resize(frame_height, vector < char > (frame_width, ' '));
    }
    frame_color.resize(frame_count);
    for (auto & frame_col: frame_color) {
        frame_col.resize(frame_height, vector < int > (frame_width, 0));
    }
}

void processMouseInput(MOUSE_EVENT_RECORD mer, int& x, int& y, bool& painting, bool& erasing, char& choice, int frame_width, int frame_height, bool line_tool, bool rectangle_tool) {
    int y_correction = 4;
    int x_correction = 9;
    int mouse_x = mer.dwMousePosition.X - x_correction;
    int mouse_y = mer.dwMousePosition.Y - y_correction;
    switch(mer.dwEventFlags) {
    case 0:
        if(mer.dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED) {
            painting = true;
            erasing = false;
            if(mouse_x < frame_width && mouse_x >= 0 && mouse_y < frame_height && mouse_y >= 0) {
                x = mouse_x;
                y = mouse_y;
                choice = 'P';
            }
        } else if(mer.dwButtonState & RIGHTMOST_BUTTON_PRESSED) {
            painting = false;
            erasing = true;
            if(mouse_x < frame_width && mouse_x >= 0 && mouse_y < frame_height && mouse_y >= 0) {
                x = mouse_x;
                y = mouse_y;
                choice = 'O';
            }
        } else {
            painting = false;
            erasing = false;
        }
        break;
    case MOUSE_MOVED:
        if(mouse_x < frame_width && mouse_x >= 0 && mouse_y < frame_height && mouse_y >= 0) {
            x = mouse_x;
            y = mouse_y;
        }
        if(painting) {
            if(mouse_x < frame_width && mouse_x >= 0 && mouse_y < frame_height && mouse_y >= 0) {
                x = mouse_x;
                y = mouse_y;
                choice = 'P';
            }
        } else if(erasing) {
            if(mouse_x < frame_width && mouse_x >= 0 && mouse_y < frame_height && mouse_y >= 0) {
                x = mouse_x;
                y = mouse_y;
                choice = 'O';
            }
        }
        break;
    default:
        break;
    }
}
