#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cctype>
#include <cstdlib>

using namespace std;

struct State
{
    string abbreviation;
    string full_name;

    State() :
        abbreviation(""), full_name("")
    {}

    State(string abv, string name) :
        abbreviation(abv), full_name(name)
    {}
};

struct Place
{
    int numeric_code;
    string state_abbreviation;
    string name;
    int population;
    double area;
    double latitude;
    double longitude;
    int intersection_code;
    double distance_intersection;

    Place() :
        numeric_code(0), state_abbreviation(""), name(""),
        population(0), area(0.0), latitude(0.0), longitude(0.0),
        intersection_code(0), distance_intersection(0.0)
    {}

    Place(int nc, string state_abbr, string n, int pop, double ar,
        double lat, double lon, int ic, double dist) :
        numeric_code(nc), state_abbreviation(state_abbr), name(n),
        population(pop), area(ar), latitude(lat), longitude(lon),
        intersection_code(ic), distance_intersection(dist)
    {}
};

struct HashNode_Array
{
    Place data;
    HashNode_Array* next;

    HashNode_Array() : next(nullptr) {}
};

//Hash table structure using array with dynamic resizing (instead of relying on file size)
struct HashTable
{
    HashNode_Array** buckets_Linked_List;
    int size;
    int count;

    HashTable() : buckets_Linked_List(nullptr), size(0), count(0) {}
};

HashTable named_places_hashtable;
State states[100];

int num_states = 0;

// Removes leading and trailing whitespace from a string
string trim_whitespace(const string& text)
{
    int first_non_space = -1;
    int last_non_space = -1;
    int text_size = text.length();

    for (int i = 0; i < text_size; i++)
    {
        if (text[i] != ' ')
        {
            if (first_non_space == -1)
                first_non_space = i;
            last_non_space = i;
        }
    }

    if (first_non_space == -1)
        return "";

    return text.substr(first_non_space, last_non_space - first_non_space + 1);
}

//See if a string represents a valid number using strtod
bool is_valid_number(const string& str)
{
    if (str.empty())
        return false;

    char* endptr = nullptr;
    strtod(str.c_str(), &endptr);

    // Valid if we consumed the entire string
    return (endptr != str.c_str() && *endptr == '\0');
}

// Gets integer at current_pos (right before an int)
int get_integer(const string& line, int& current_pos)
{
    int start = current_pos;
    int int_size = line.length();

    while (current_pos < int_size && isdigit(line[current_pos]))
        current_pos++;

    string num_str = line.substr(start, current_pos - start);

    if (is_valid_number(num_str))
        return stoi(num_str);
    return 0;
}

//Gets double at current_pos (right before a double)
double get_double(const string& line, int& current_pos, bool negative = false)
{
    int start = current_pos;
    int double_size = line.length();

    if (negative && current_pos < double_size && line[current_pos] == '-')
        current_pos++;

    while (current_pos < double_size && (isdigit(line[current_pos]) || line[current_pos] == '.'))
        current_pos++;

    string num_str = line.substr(start, current_pos - start);

    if (is_valid_number(num_str))
        return stod(num_str);
    return 0.0;
}

//Skips whitespace characters in a line
void skip_spaces(const string& line, int& current_pos)
{
    int spaces_size = line.length();
    while (current_pos < spaces_size && line[current_pos] == ' ')
        current_pos++;
}

// Finds full state name by abbreviation
string get_state_full_name(const string& abv)
{
    for (int i = 0; i < num_states; i++)
    {
        if (states[i].abbreviation == abv)
            return states[i].full_name;
    }
    return "";
}

//Parses state line (format: "AB Full Name")
State parse_state_line(const string& line)
{
    int space_pos = line.find(' ');
    if (space_pos == -1)
        return State();

    string abv = line.substr(0, space_pos);
    string name = line.substr(space_pos + 1);

    return State(abv, name);
}

//Loads state data from states.txt file
void load_states_file()
{
    ifstream file("states.txt");
    string line;

    if (!file.is_open())
        return;

    while (getline(file, line))
    {
        if (line.empty())
            continue;

        State state = parse_state_line(line);
        if (state.abbreviation.empty())
            continue;

        if (num_states < 100)
            states[num_states++] = state;
    }

    file.close();
}

// PLACE PARSING SEPARATE FUNCTIONS

//Checks if next number in line has a decimal point
bool has_decimal_in_next_number(const string& line, int pos)
{
    int data_size = line.length();
    while (pos < data_size && (isdigit(line[pos]) || line[pos] == '.'))
    {
        if (line[pos] == '.')
            return true;
        pos++;
    }
    return false;
}

// Parses a single line from named-places.txt into a Place object
// Reports errors if parsing fails
Place analyze_data_line(const string& line, int line_number)
{
    int pos = 0;
    int data_size = line.length();

    // 1. Numeric code
    if (pos >= data_size || !isdigit(line[pos]))
    {
        cerr << "Error parsing numeric code at line " << line_number << "\n";
        return Place();
    }
    int numeric_code = get_integer(line, pos);
    if (numeric_code == 0)
    {
        cerr << "Error parsing numeric code at line " << line_number << "\n";
        return Place();
    }

    // 2. State abbreviation (2 characters)
    if (pos + 1 >= data_size)
    {
        cerr << "Error parsing state abbreviation at line " << line_number << "\n";
        return Place();
    }
    string state_abv = line.substr(pos, 2);
    pos += 2;

    // 3. Place name (until next digit)
    int name_start = pos;
    while (pos < data_size && !isdigit(line[pos]))
        pos++;
    string place_name = trim_whitespace(line.substr(name_start, pos - name_start));

    if (place_name.empty())
    {
        cerr << "Error: empty place name at line " << line_number << "\n";
        return Place();
    }

    // 4. Population
    if (pos >= data_size || !isdigit(line[pos]))
    {
        cerr << "Error parsing population at line " << line_number << "\n";
        return Place();
    }
    int population = get_integer(line, pos);

    skip_spaces(line, pos);

    // 5. Area
    if (pos >= data_size || (!isdigit(line[pos]) && line[pos] != '.'))
    {
        cerr << "Error parsing area at line " << line_number << "\n";
        return Place();
    }
    double area = get_double(line, pos);
    if (area == 0.0)
    {
        cerr << "Error parsing area at line " << line_number << "\n";
        return Place();
    }

    skip_spaces(line, pos);

    // 6. Latitude
    if (pos >= data_size || (!isdigit(line[pos]) && line[pos] != '.'))
    {
        cerr << "Error parsing latitude at line " << line_number << "\n";
        return Place();
    }
    double latitude = get_double(line, pos);
    if (latitude == 0.0)
    {
        cerr << "Error parsing latitude at line " << line_number << "\n";
        return Place();
    }

    skip_spaces(line, pos);

    // 7. Longitude (can be negative)
    if (pos >= data_size || (!isdigit(line[pos]) && line[pos] != '-' && line[pos] != '.'))
    {
        cerr << "Error parsing longitude at line " << line_number << "\n";
        return Place();
    }
    double longitude = get_double(line, pos, true);
    if (longitude == 0.0)
    {
        cerr << "Error parsing longitude at line " << line_number << "\n";
        return Place();
    }

    skip_spaces(line, pos);

    // 8. Intersection code
    int intersection_code = 0;
    if (pos < data_size && !has_decimal_in_next_number(line, pos))
    {
        intersection_code = get_integer(line, pos);
        skip_spaces(line, pos);
    }

    // 9. Distance to intersection
    if (pos >= data_size || (!isdigit(line[pos]) && line[pos] != '.'))
    {
        cerr << "Error parsing distance at line " << line_number << "\n";
        return Place();
    }
    double distance = get_double(line, pos);
    if (distance == 0.0)
    {
        cerr << "Error parsing distance at line " << line_number << "\n";
        return Place();
    }

    return Place(numeric_code, state_abv, place_name,
        population, area, latitude, longitude,
        intersection_code, distance
    );
}
//==============================

// HASH TABLE FUNCTIONS

// Calculates hash value for a place name
int calc_hash(const string& place_name)
{
    int total = 0;
    int place_name_size = place_name.length();

    for (int i = 0; i < place_name_size; i++)
        total += (int)place_name[i];
    return total % named_places_hashtable.size;
}

//Start hash table with given size
void start_hashtable(int size)
{
    named_places_hashtable.buckets_Linked_List = new HashNode_Array * [size];
    named_places_hashtable.size = size;
    named_places_hashtable.count = 0;

    for (int i = 0; i < size; i++)
        named_places_hashtable.buckets_Linked_List[i] = nullptr;
}

//resize hash table
void resize_hash_table();

// Inserts a place into the hash table
void insert_place(const Place& place)
{
    int index = calc_hash(place.name);

    HashNode_Array* node = new HashNode_Array();
    node->data = place;
    node->next = named_places_hashtable.buckets_Linked_List[index];
    named_places_hashtable.buckets_Linked_List[index] = node;

    named_places_hashtable.count++;

    // Resize if load factor exceeds 0.75
    if (named_places_hashtable.size > 0 &&
        (double)named_places_hashtable.count / named_places_hashtable.size > 0.75)
        resize_hash_table();
}

//Resizes hash table by doubling its size and reusing existing nodes
void resize_hash_table()
{
    HashNode_Array** old_buckets_Linked_List = named_places_hashtable.buckets_Linked_List;
    int old_hashtable_size = named_places_hashtable.size;
    int new_hashtable_size = old_hashtable_size * 2;

    //Create new table
    HashNode_Array** new_buckets_Linked_List = new HashNode_Array * [new_hashtable_size];
    for (int i = 0; i < new_hashtable_size; i++)
        new_buckets_Linked_List[i] = nullptr;

    named_places_hashtable.buckets_Linked_List = new_buckets_Linked_List;
    named_places_hashtable.size = new_hashtable_size;
    named_places_hashtable.count = 0;

    //Rehash all elements, reusing existing nodes
    for (int i = 0; i < old_hashtable_size; i++)
    {
        HashNode_Array* current = old_buckets_Linked_List[i];
        while (current != nullptr)
        {
            HashNode_Array* next = current->next;

            // Reuse the node instead of creating new one
            int new_index = calc_hash(current->data.name);
            current->next = named_places_hashtable.buckets_Linked_List[new_index];
            named_places_hashtable.buckets_Linked_List[new_index] = current;
            named_places_hashtable.count++;

            current = next;
        }
    }
    delete[] old_buckets_Linked_List;
}

//=============================

//Loads place data from named-places.txt file
void load_named_places_file()
{
    ifstream file("named-places.txt");
    string line;
    int line_number = 0;

    if (!file.is_open())
    {
        cerr << "Error: Could not open named-places.txt\n";
        return;
    }

    while (getline(file, line))
    {
        line_number++;
        if (line.empty())
            continue;

        Place place = analyze_data_line(line, line_number);
        if (!place.name.empty())
            insert_place(place);
    }

    file.close();
}

// Finds all places with a given name - returns vector
vector<Place> get_all_places(const string& name)
{
    vector<Place> results;
    int index = calc_hash(name);
    HashNode_Array* current = named_places_hashtable.buckets_Linked_List[index];

    while (current != nullptr)
    {
        if (current->data.name == name)
            results.push_back(current->data);
        current = current->next;
    }

    return results;
}

//Finds a specific place by name and state
Place* find_specific_place(const string& name, const string& state)
{
    int index = calc_hash(name);
    HashNode_Array* current = named_places_hashtable.buckets_Linked_List[index];

    while (current != nullptr)
    {
        if (current->data.name == name && current->data.state_abbreviation == state)
            return &(current->data);
        current = current->next;
    }
    return nullptr;
}

//destroy hash table
void destroy_hash_table()
{
    for (int i = 0; i < named_places_hashtable.size; i++)
    {
        HashNode_Array* current = named_places_hashtable.buckets_Linked_List[i];
        while (current != nullptr)
        {
            HashNode_Array* next = current->next;
            delete current;
            current = next;
        }
    }
    delete[] named_places_hashtable.buckets_Linked_List;
    named_places_hashtable.buckets_Linked_List = nullptr;
}

// COMMAND FUNCTIONS

//Handles 'N placename' command - lists all states with that place name
void handle_N_command(const string& place_name)
{
    vector<Place> found_places = get_all_places(place_name);

    if (found_places.empty())
    {
        cout << "No places found with that name." << '\n';
        return;
    }

    vector<string> displayed_states;

    for (size_t i = 0; i < found_places.size(); i++)
    {
        string current_state_abv = found_places[i].state_abbreviation;
        bool already_displayed = false;

        for (size_t j = 0; j < displayed_states.size(); j++)
        {
            if (displayed_states[j] == current_state_abv)
            {
                already_displayed = true;
                break;
            }
        }

        if (!already_displayed)
        {
            cout << current_state_abv << " " << get_state_full_name(current_state_abv) << '\n';
            displayed_states.push_back(current_state_abv);
        }
    }
}

//Handles 'S placename state' command - shows all info for specific place
void handle_S_command(const string& place_name, const string& state_abv)
{
    Place* specific_place = find_specific_place(place_name, state_abv);

    if (specific_place != nullptr)
    {
        cout << "Name: " << specific_place->name << '\n';
        cout << "State: " << specific_place->state_abbreviation << " ("
             << get_state_full_name(specific_place->state_abbreviation) << ")\n";
        cout << "Numeric Code: " << specific_place->numeric_code << '\n';
        cout << "Population: " << specific_place->population << '\n';
        cout << "Area: " << specific_place->area << '\n';
        cout << "Latitude: " << specific_place->latitude << '\n';
        cout << "Longitude: " << specific_place->longitude << '\n';
        cout << "Intersection Code: " << specific_place->intersection_code << '\n';
        cout << "Distance to Intersection: " << specific_place->distance_intersection << '\n';
    }
    else
        cout << "Place not found: " << place_name << " in " << state_abv << '\n';
}

//Parses 'S' command to extract place name and state (handles spaces in names)
void parse_S_command(const string& args)
{
    string trimmed = trim_whitespace(args);
    int last_space = trimmed.find_last_of(' ');

    if (last_space == -1 || last_space == trimmed.length() - 1)
    {
        cout << "Invalid format. Use: S placename state" << '\n';
        return;
    }

    string place_name = trim_whitespace(trimmed.substr(0, last_space));
    string state = trim_whitespace(trimmed.substr(last_space + 1));

    handle_S_command(place_name, state);
}

int main()
{
    //Start
    start_hashtable(7);
    load_states_file();
    load_named_places_file();

    //Interactive loop
    string command_line;
    while (true)
    {
        cout << "Enter command (N/S/Q): ";
        getline(cin, command_line);

        if (command_line.empty())
            continue;

        char cmd = command_line[0];

        if (cmd == 'Q' || cmd == 'q')
            break;

        else if (cmd == 'N' || cmd == 'n')
        {
            string place_name = trim_whitespace(command_line.substr(1));
            handle_N_command(place_name);
        }
        else if (cmd == 'S' || cmd == 's')
            parse_S_command(command_line.substr(1));
        else
            cout << "Invalid command. Use: N, S, or Q." << '\n';
    }
    destroy_hash_table();
    return 0;
}
