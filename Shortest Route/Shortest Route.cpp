#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cmath> 
#include <limits> 
#include <iomanip> 
#include <cctype> 
#include <cstdlib>

using namespace std;

#define PI 3.14159265358979323846

struct connection
{
    string name_road;
    string type_road;
    int A_start;
    int B_end;
    double length;
    string direction; //N/S/E/W direction

    connection() : name_road(""), type_road(""), A_start(0), B_end(0), length(0.0), direction("") {}

    connection(string name_r, string type_r, int start, int end, double len, string dir) :
        name_road(name_r), type_road(type_r), A_start(start), B_end(end), length(len), direction(dir) {}
};

struct intersection
{
    int intersection_number;
    double longitude;
    double latitude;
    double distance_nearest;
    string states_abv;
    string named_places;
    vector<connection> leading_roads;

    // Pathfinding fields
    double dist_from_source;
    bool visited;
    int previous_intersection;
    int previous_connection_index;

    intersection() : intersection_number(0), longitude(0.0), latitude(0.0),
        distance_nearest(0.0), states_abv(""), named_places(""),
        dist_from_source(numeric_limits<double>::infinity()), visited(false),
        previous_intersection(-1), previous_connection_index(-1) {}

    intersection(int its_num, double lon, double lat, double dist_near, string s_abv, string named_p) :
        intersection_number(its_num), longitude(lon), latitude(lat),
        distance_nearest(dist_near), states_abv(s_abv), named_places(named_p),
        dist_from_source(numeric_limits<double>::infinity()), visited(false),
        previous_intersection(-1), previous_connection_index(-1) {}
};

struct State
{
    string abbreviation;
    string full_name;

    State() : abbreviation(""), full_name("") {}
    State(string abbr, string name) : abbreviation(abbr), full_name(name) {}
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

    Place() : numeric_code(0), state_abbreviation(""), name(""),
        population(0), area(0.0), latitude(0.0), longitude(0.0),
        intersection_code(0), distance_intersection(0.0) {}

    Place(int nc, string state_abbr, string n, int pop, double ar,
        double lat, double lon, int ic, double dist) :
        numeric_code(nc), state_abbreviation(state_abbr), name(n),
        population(pop), area(ar), latitude(lat), longitude(lon),
        intersection_code(ic), distance_intersection(dist) {}
};

struct HashNode
{
    Place data;
    HashNode* next;

    HashNode() : next(nullptr) {}
};

struct HashTable
{
    HashNode** buckets;
    int size;
    int count;

    HashTable() : buckets(nullptr), size(0), count(0) {}
};

struct PathStep
{
    int from_intersection;
    int to_intersection;
    string road_name;
    string road_type;
    double distance;
    string direction;
    string destination_place_name;
    string destination_place_state_abv;

    PathStep() : from_intersection(-1), to_intersection(-1), road_name(""),
        road_type(""), distance(0.0), direction(""), destination_place_name(""),
        destination_place_state_abv("") {}
};

string trim_whitespace(const string& text)
{
    size_t start_pos = 0;
    while (start_pos < text.length() && text[start_pos] == ' ')
        start_pos++;

    size_t end_pos = text.length();
    while (end_pos > start_pos && text[end_pos - 1] == ' ')
        end_pos--;

    if (start_pos >= end_pos) return "";
    return text.substr(start_pos, end_pos - start_pos);
}


string remove_carriage_return(const string& line)
{
    if (!line.empty() && line.back() == '\r')
        return line.substr(0, line.length() - 1);
    return line;
}

bool is_valid_state_abbreviation(const string& str)
{
    if (str.length() != 2) return false;
    return (str[0] >= 'A' && str[0] <= 'Z') && (str[1] >= 'A' && str[1] <= 'Z');
}


string extract_last_word(const string& text)
{
    size_t last_space = text.find_last_of(' ');
    if (last_space == string::npos)
        return text;
    return text.substr(last_space + 1);
}

string remove_last_word(const string& text)
{
    size_t last_space = text.find_last_of(' ');
    if (last_space == string::npos)
        return "";
    return trim_whitespace(text.substr(0, last_space));
}

double calculate_distance(double lat1, double lon1, double lat2, double lon2)
{
    double dlat = lat2 - lat1;
    double dlon = lon2 - lon1;
    return sqrt(dlat * dlat + dlon * dlon);
}

State parse_state_line(const string& line)
{
    size_t space_pos = line.find(' ');
    if (space_pos == string::npos)
        return State();
    string abbr = line.substr(0, space_pos);
    string name = line.substr(space_pos + 1);
    return State(abbr, name);
}

void load_states(vector<State>& states_vec)
{
    ifstream file("states.txt");
    if (!file.is_open())
        return;

    string line;
    while (getline(file, line))
    {
        line = remove_carriage_return(line);
        if (line.empty())
            continue;

        State state = parse_state_line(line);
        if (!state.abbreviation.empty())
            states_vec.push_back(state);
    }
    file.close();
}

string get_state_full_name(const vector<State>& states_vec, const string& abbreviation)
{
    for (size_t i = 0; i < states_vec.size(); i++)
    {
        if (states_vec[i].abbreviation == abbreviation)
            return states_vec[i].full_name;
    }
    return "";
}

// Simple N/S/E/W direction calculation
string calc_simple_direction(intersection& start, intersection& end)
{
    double lat_diff = end.latitude - start.latitude;
    double lon_diff = end.longitude - start.longitude;

    string direction = "";
    if (lat_diff > 0)
        direction += "N";
    else if (lat_diff < 0)
        direction += "S";

    if (lon_diff > 0)
        direction += "E";
    else if (lon_diff < 0)
        direction += "W";

    if (direction == "")
        direction = "Same";
    return direction;
}


void read_intersections(string filename, vector<intersection>& intersections_vec)
{
    ifstream intersection_file(filename);
    string line;

    if (intersection_file.is_open())
    {
        int intersection_count = 0;
        while (getline(intersection_file, line))
        {
            line = remove_carriage_return(line);
            if (line.empty()) continue;

            stringstream line_ss(line);
            double longitude, latitude, distance_nearest;
            string states_abv_str, named_places_str;

            line_ss >> longitude >> latitude >> distance_nearest >> states_abv_str;
            getline(line_ss, named_places_str);

            if (!named_places_str.empty() && named_places_str[0] == ' ')
                named_places_str = named_places_str.substr(1);

            intersection new_intersection(intersection_count, longitude, latitude,
                distance_nearest, states_abv_str, named_places_str);
            intersections_vec.push_back(new_intersection);
            intersection_count++;
        }
    }
    intersection_file.close();
}

void read_connections(string filename, vector<intersection>& intersections_vec)
{
    ifstream connection_file(filename);
    string line;

    if (connection_file.is_open())
    {
        while (getline(connection_file, line))
        {
            line = remove_carriage_return(line);
            if (line.empty())
                continue;

            stringstream line_ss(line);
            string name_road_str, type_road_str;
            int A_start, B_end;
            double length;

            line_ss >> name_road_str >> type_road_str >> A_start >> B_end >> length;

            if (A_start >= 0 && static_cast<size_t>(A_start) < intersections_vec.size() &&
                B_end >= 0 && static_cast<size_t>(B_end) < intersections_vec.size())
            {
                string dir = calc_simple_direction(intersections_vec[A_start], intersections_vec[B_end]);
                connection new_connection(name_road_str, type_road_str, A_start, B_end, length, dir);
                intersections_vec[A_start].leading_roads.push_back(new_connection);
            }
        }
    }
    connection_file.close();
}

void add_reverse_connections(vector<intersection>& intersections_vec)
{
    // Store all reverse connections first to avoid modifying while iterating
    vector<connection> all_reverse_connections;

    for (size_t i = 0; i < intersections_vec.size(); i++)
    {
        for (size_t j = 0; j < intersections_vec[i].leading_roads.size(); j++)
        {
            const connection& conn = intersections_vec[i].leading_roads[j];
            int from = conn.A_start;
            int to = conn.B_end;

            // Check if reverse connection already exists
            bool reverse_exists = false;
            for (size_t k = 0; k < intersections_vec[to].leading_roads.size(); k++)
            {
                if (intersections_vec[to].leading_roads[k].B_end == from &&
                    intersections_vec[to].leading_roads[k].name_road == conn.name_road)
                {
                    reverse_exists = true;
                    break;
                }
            }

            // Only add reverse if it doesn't exist
            if (!reverse_exists)
            {
                string reverse_dir = calc_simple_direction(intersections_vec[to], intersections_vec[from]);
                connection reverse_conn(conn.name_road, conn.type_road, to, from, conn.length, reverse_dir);
                all_reverse_connections.push_back(reverse_conn);
            }
        }
    }

    // Now add all reverse connections
    for (size_t i = 0; i < all_reverse_connections.size(); i++)
    {
        int start_idx = all_reverse_connections[i].A_start;
        if (start_idx >= 0 && static_cast<size_t>(start_idx) < intersections_vec.size())
        {
            intersections_vec[start_idx].leading_roads.push_back(all_reverse_connections[i]);
        }
    }
}

void make_graph(vector<intersection>& intersections_vec, string intersection_file, string connection_file)
{
    read_intersections(intersection_file, intersections_vec);
    read_connections(connection_file, intersections_vec);
    add_reverse_connections(intersections_vec);
}

//HashTable Function
int calculate_hash(const string& name, int table_size)
{
    int total = 0;
    for (size_t i = 0; i < name.length(); i++)
        total += static_cast<int>(name[i]);
    return total % table_size;
}

void init_hash_table(HashTable& table, int size)
{
    table.buckets = new HashNode * [size];
    table.size = size;
    table.count = 0;

    for (int i = 0; i < size; i++)
        table.buckets[i] = nullptr;
}

void insert_place(HashTable& table, const Place& place);

void resize_hash_table(HashTable& table)
{
    HashNode** old_buckets = table.buckets;
    int old_size = table.size;
    int new_size = old_size * 2;

    table.buckets = new HashNode * [new_size];
    table.size = new_size;
    table.count = 0;

    for (int i = 0; i < new_size; i++)
        table.buckets[i] = nullptr;

    for (int i = 0; i < old_size; i++)
    {
        HashNode* current = old_buckets[i];
        while (current != nullptr)
        {
            HashNode* next = current->next;
            insert_place(table, current->data);
            delete current;
            current = next;
        }
    }

    delete[] old_buckets;
}

void insert_place(HashTable& table, const Place& place)
{
    int index = calculate_hash(place.name, table.size);

    HashNode* node = new HashNode();
    node->data = place;
    node->next = table.buckets[index];
    table.buckets[index] = node;
    table.count++;

    if (table.size > 0 && (static_cast<double>(table.count) / table.size) > 0.75)
        resize_hash_table(table);
}

Place parse_place_line(const string& line)
{
    const char* p = line.c_str();

    int numeric_code = 0;
    while (*p && (*p >= '0' && *p <= '9'))
    {
        numeric_code = numeric_code * 10 + (*p - '0');
        p++;
    }

    if (*p == '\0' || *(p + 1) == '\0')
        return Place();
    string state_abbr(p, 2);
    p += 2;

    const char* name_start = p;
    while (*p && !(*p >= '0' && *p <= '9')) p++;
    string place_name = trim_whitespace(string(name_start, p - name_start));
    if (place_name.empty()) return Place();

    int population = 0;
    while (*p && (*p >= '0' && *p <= '9'))
    {
        population = population * 10 + (*p - '0');
        p++;
    }
    while (*p == ' ') p++;

    double area = 0.0;
    const char* num_start = p;
    while (*p && ((*p >= '0' && *p <= '9') || *p == '.')) p++;
    if (p > num_start) area = stod(string(num_start, p - num_start));
    while (*p == ' ') p++;

    double latitude = 0.0;
    num_start = p;
    while (*p && ((*p >= '0' && *p <= '9') || *p == '.')) p++;
    if (p > num_start) latitude = stod(string(num_start, p - num_start));
    while (*p == ' ') p++;

    double longitude = 0.0;
    num_start = p;
    if (*p == '-') p++;
    while (*p && ((*p >= '0' && *p <= '9') || *p == '.')) p++;
    if (p > num_start) longitude = stod(string(num_start, p - num_start));
    while (*p == ' ') p++;

    int intersection_code = 0;
    const char* temp_p = p;
    bool has_decimal = false;
    while (*temp_p && ((*temp_p >= '0' && *temp_p <= '9') || *temp_p == '.'))
    {
        if (*temp_p == '.')
        {
            has_decimal = true;
            break;
        }
        temp_p++;
    }

    if (!has_decimal && *p && (*p >= '0' && *p <= '9'))
    {
        while (*p && (*p >= '0' && *p <= '9'))
        {
            intersection_code = intersection_code * 10 + (*p - '0');
            p++;
        }
        while (*p == ' ') p++;
    }

    double distance = 0.0;
    num_start = p;
    while (*p && ((*p >= '0' && *p <= '9') || *p == '.')) p++;
    if (p > num_start) distance = stod(string(num_start, p - num_start));
    return Place(numeric_code, state_abbr, place_name, population,
        area, latitude, longitude, intersection_code, distance);
}

void load_places(HashTable& table)
{
    ifstream file("named-places.txt");
    if (!file.is_open())
    {
        cerr << "Error: Could not open named-places.txt\n";
        return;
    }

    string line;
    while (getline(file, line))
    {
        line = remove_carriage_return(line);
        if (line.empty())
            continue;

        Place place = parse_place_line(line);
        if (!place.name.empty())
            insert_place(table, place);
    }
    file.close();
}

vector<Place> find_places_by_name(const HashTable& table, const string& name)
{
    vector<Place> results;
    int index = calculate_hash(name, table.size);
    HashNode* current = table.buckets[index];

    while (current != nullptr)
    {
        if (current->data.name == name)
            results.push_back(current->data);
        current = current->next;
    }
    return results;
}

Place* find_place_by_name_and_state(HashTable& table, const string& name, const string& state)
{
    int index = calculate_hash(name, table.size);
    HashNode* current = table.buckets[index];

    while (current != nullptr)
    {
        if (current->data.name == name && current->data.state_abbreviation == state)
            return &(current->data);
        current = current->next;
    }
    return nullptr;
}

void destroy_hash_table(HashTable& table)
{
    for (int i = 0; i < table.size; i++)
    {
        HashNode* current = table.buckets[i];
        while (current != nullptr)
        {
            HashNode* next = current->next;
            delete current;
            current = next;
        }
    }
    delete[] table.buckets;
    table.buckets = nullptr;
}

//Direction Calculation

double calculate_angle(double lat1, double lon1, double lat2, double lon2)
{
    double dlat = lat2 - lat1;
    double dlon = lon2 - lon1;

    double angle = atan2(dlon, dlat) * 180.0 / PI;
    if (angle < 0) angle += 360.0;
    return angle;
}

string angle_to_compass(double angle)
{
    // Normalize angle to 0-360
    while (angle < 0) angle += 360;
    while (angle >= 360) angle -= 360;

    // 8 compass directions, each spanning 45 degrees
    // N: 337.5-22.5, NE: 22.5-67.5, E: 67.5-112.5, etc.
    if (angle >= 337.5 || angle < 22.5)
        return "North";
    else if (angle >= 22.5 && angle < 67.5)
        return "North-East";
    else if (angle >= 67.5 && angle < 112.5)
        return "East";
    else if (angle >= 112.5 && angle < 157.5)
        return "South-East";
    else if (angle >= 157.5 && angle < 202.5)
        return "South";
    else if (angle >= 202.5 && angle < 247.5)
        return "South-West";
    else if (angle >= 247.5 && angle < 292.5)
        return "West";
    else return "North-West";
}

string calc_direction_compass(const intersection& start, const intersection& end)
{
    double angle = calculate_angle(start.latitude, start.longitude,
        end.latitude, end.longitude);
    return angle_to_compass(angle);
}

void display_state_options(const vector<Place>& places, const vector<State>& states_vec)
{
    vector<string> displayed;

    for (size_t i = 0; i < places.size(); i++)
    {
        bool already_shown = false;
        for (size_t j = 0; j < displayed.size(); j++)
        {
            if (displayed[j] == places[i].state_abbreviation)
            {
                already_shown = true;
                break;
            }
        }

        if (!already_shown)
        {
            string full_name = get_state_full_name(states_vec, places[i].state_abbreviation);
            cout << displayed.size() + 1 << ". " << places[i].state_abbreviation
                << " " << full_name << "\n";
            displayed.push_back(places[i].state_abbreviation);
        }
    }
}

int get_user_state_choice(size_t num_options)
{
    cout << "Enter choice (1-" << num_options << "): ";
    int choice;
    cin >> choice;
    cin.ignore(1000, '\n');

    if (choice >= 1 && static_cast<size_t>(choice) <= num_options)
        return choice - 1;
    return -1;
}

Place select_place_from_list(const vector<Place>& places, const vector<State>& states_vec)
{
    vector<string> unique_states;

    for (size_t i = 0; i < places.size(); i++)
    {
        bool found = false;
        for (size_t j = 0; j < unique_states.size(); j++)
        {
            if (unique_states[j] == places[i].state_abbreviation)
            {
                found = true;
                break;
            }
        }
        if (!found) {
            unique_states.push_back(places[i].state_abbreviation);
        }
    }

    if (unique_states.size() == 1) {
        for (size_t i = 0; i < places.size(); i++)
        {
            if (places[i].state_abbreviation == unique_states[0])
                return places[i];
        }
    }

    cout << "Multiple states found. Please select:\n";
    display_state_options(places, states_vec);

    int choice = get_user_state_choice(unique_states.size());
    if (choice >= 0 && static_cast<size_t>(choice) < unique_states.size())
    {
        string selected_state = unique_states[choice];
        for (size_t i = 0; i < places.size(); i++)
        {
            if (places[i].state_abbreviation == selected_state)
                return places[i];
        }
    }

    return Place();
}

bool get_place_with_state_hint(const string& user_input,
    HashTable& place_table,
    vector<State>& states_vec,
    Place& selected_place)
{
    string last_word = extract_last_word(user_input);

    if (is_valid_state_abbreviation(last_word))
    {
        string place_name = remove_last_word(user_input);
        if (!place_name.empty())
        {
            Place* found = find_place_by_name_and_state(place_table, place_name, last_word);
            if (found != nullptr)
            {
                selected_place = *found;
                return true;
            }
        }
    }

    vector<Place> places = find_places_by_name(place_table, user_input);
    if (places.empty())
    {
        cout << "Place not found: " << user_input << "\n";
        return false;
    }

    selected_place = select_place_from_list(places, states_vec);
    return !selected_place.name.empty();
}

//Dijkstra Algorithm

void initialize_dijkstra(vector<intersection>& intersections_vec)
{
    for (size_t i = 0; i < intersections_vec.size(); i++)
    {
        intersections_vec[i].dist_from_source = numeric_limits<double>::infinity();
        intersections_vec[i].visited = false;
        intersections_vec[i].previous_intersection = -1;
        intersections_vec[i].previous_connection_index = -1;
    }
}

int find_min_unvisited(const vector<intersection>& intersections_vec)
{
    int min_index = -1;
    double min_dist = numeric_limits<double>::infinity();

    for (size_t i = 0; i < intersections_vec.size(); i++)
    {
        if (!intersections_vec[i].visited && intersections_vec[i].dist_from_source < min_dist)
        {
            min_dist = intersections_vec[i].dist_from_source;
            min_index = static_cast<int>(i);
        }
    }

    return min_index;
}

void dijkstra(vector<intersection>& intersections_vec, int start_index)
{
    if (intersections_vec.empty())
    {
        cerr << "Error: Graph is empty, cannot run Dijkstra's.\n";
        return;
    }

    initialize_dijkstra(intersections_vec);
    intersections_vec[start_index].dist_from_source = 0.0;

    for (size_t count = 0; count < intersections_vec.size(); count++)
    {
        int u = find_min_unvisited(intersections_vec);

        if (u == -1)
            break;

        intersections_vec[u].visited = true;

        for (size_t j = 0; j < intersections_vec[u].leading_roads.size(); j++)
        {
            const connection& conn = intersections_vec[u].leading_roads[j];
            int v = conn.B_end;

            // Defensive check for v bounds
            if (v >= 0 && static_cast<size_t>(v) < intersections_vec.size())
            {
                if (!intersections_vec[v].visited)
                {
                    double new_dist = intersections_vec[u].dist_from_source + conn.length;

                    if (new_dist < intersections_vec[v].dist_from_source)
                    {
                        intersections_vec[v].dist_from_source = new_dist;
                        intersections_vec[v].previous_intersection = u;
                        intersections_vec[v].previous_connection_index = static_cast<int>(j);
                    }
                }
            }
        }
    }
}

bool reconstruct_path(const vector<intersection>& intersections_vec,
    int start_index,
    int end_index,
    vector<PathStep>& path)
{
    if (intersections_vec[end_index].dist_from_source == numeric_limits<double>::infinity())
        return false;

    vector<PathStep> reverse_path;
    int current = end_index;

    while (current != start_index)
    {
        int prev = intersections_vec[current].previous_intersection;
        if (prev == -1) return false;

        int conn_index = intersections_vec[current].previous_connection_index;
        if (conn_index == -1 || static_cast<size_t>(conn_index) >= intersections_vec[prev].leading_roads.size())
        {
            cerr << "Invalid connection index during path reconstruction.\n";
            return false;
        }
        const connection& conn = intersections_vec[prev].leading_roads[conn_index];

        PathStep step;
        step.from_intersection = prev;
        step.to_intersection = current;
        step.road_name = conn.name_road;
        step.road_type = conn.type_road;
        step.distance = conn.length;
        step.direction = calc_direction_compass(intersections_vec[prev], intersections_vec[current]);
        step.destination_place_name = intersections_vec[current].named_places;
        step.destination_place_state_abv = intersections_vec[current].states_abv;

        reverse_path.push_back(step);
        current = prev;
    }

    // Manually reverse to get correct order 
    size_t reverse_path_size = reverse_path.size();
    for (size_t i = 0; i < reverse_path_size / 2; i++)
    {
        PathStep temp = reverse_path[i];
        reverse_path[i] = reverse_path[reverse_path_size - 1 - i];
        reverse_path[reverse_path_size - 1 - i] = temp;
    }
    path = reverse_path;

    return true;
}

void print_path(const vector<PathStep>& path,
    const Place& start_place,
    const Place& end_place)
{
    cout << fixed << setprecision(2);

    cout << "Start at " << start_place.distance_intersection
        << " miles from " << start_place.name << " " << start_place.state_abbreviation << "\n";

    double total_distance = start_place.distance_intersection;

    for (size_t i = 0; i < path.size(); i++)
    {
        const PathStep& step = path[i];
        string road_full = step.road_name;
        if (!step.road_type.empty() && step.road_type != "?")
            road_full = step.road_type + "-" + step.road_name;

        cout << "then take " << road_full << " " << step.distance
            << " miles " << step.direction << " to ";

        if (!step.destination_place_name.empty())
            cout << step.destination_place_name << " " << step.destination_place_state_abv << "\n";
        else
            cout << "intersection " << step.to_intersection << "\n";

        total_distance += step.distance;
    }

    total_distance += end_place.distance_intersection;

    if (!path.empty())
    {
        cout << "then continue " << end_place.distance_intersection
            << " miles to " << end_place.name << " " << end_place.state_abbreviation << "\n";
    }
    cout << "\nTotal distance: " << total_distance << " miles.\n";
}

int find_nearest_intersection(double latitude, double longitude, const vector<intersection>& graph_vec);

int main()
{
    vector<State> loaded_states;
    load_states(loaded_states);

    HashTable place_table;
    init_hash_table(place_table, 7);
    load_places(place_table);

    vector<intersection> graph_vec;
    make_graph(graph_vec, "intersections.txt", "connections.txt");

    cout << "Shortest Path Finder\n";

    cout << "Enter starting point: ";
    string start_input;
    getline(cin, start_input);

    Place start_place;
    if (!get_place_with_state_hint(start_input, place_table, loaded_states, start_place))
    {
        destroy_hash_table(place_table);
        return 1;
    }

    cout << "Enter destination: ";
    string dest_input;
    getline(cin, dest_input);

    Place dest_place;
    if (!get_place_with_state_hint(dest_input, place_table, loaded_states, dest_place))
    {
        destroy_hash_table(place_table);
        return 1;
    }

    cout << "\nFinding shortest path from " << start_place.name << " "
        << start_place.state_abbreviation << " to " << dest_place.name
        << " " << dest_place.state_abbreviation << "...\n";

    int start_intersection_id = start_place.intersection_code;
    int end_intersection_id = dest_place.intersection_code;

    if (start_intersection_id <= 0 || static_cast<size_t>(start_intersection_id) >= graph_vec.size())
    {
        cout << "Adjusting start: Finding nearest intersection to " << start_place.name << "...\n";
        start_intersection_id = find_nearest_intersection(start_place.latitude, start_place.longitude, graph_vec);
        if (start_intersection_id != -1 && !graph_vec.empty())
        {
            start_place.distance_intersection = calculate_distance(start_place.latitude, start_place.longitude,
                graph_vec[start_intersection_id].latitude,
                graph_vec[start_intersection_id].longitude) * 69.0;
        }
    }

    if (end_intersection_id <= 0 || static_cast<size_t>(end_intersection_id) >= graph_vec.size())
    {
        cout << "Adjusting end: Finding nearest intersection to " << dest_place.name << "...\n";
        end_intersection_id = find_nearest_intersection(dest_place.latitude, dest_place.longitude, graph_vec);
        if (end_intersection_id != -1 && !graph_vec.empty())
        {
            dest_place.distance_intersection = calculate_distance(dest_place.latitude, dest_place.longitude,
                graph_vec[end_intersection_id].latitude,
                graph_vec[end_intersection_id].longitude) * 69.0;
        }
    }

    if (start_intersection_id < 0 || static_cast<size_t>(start_intersection_id) >= graph_vec.size())
    {
        cerr << "Could not determine a valid starting intersection.\n";
        destroy_hash_table(place_table);
        return 1;
    }
    if (end_intersection_id < 0 || static_cast<size_t>(end_intersection_id) >= graph_vec.size())
    {
        cerr << "Could not determine a valid destination intersection.\n";
        destroy_hash_table(place_table);
        return 1;
    }

    cout << "\nStart intersection ID: " << start_intersection_id
        << " (" << graph_vec[start_intersection_id].named_places << ")\n";
    cout << "End intersection ID: " << end_intersection_id
        << " (" << graph_vec[end_intersection_id].named_places << ")\n\n";

    dijkstra(graph_vec, start_intersection_id);

    vector<PathStep> path;
    if (reconstruct_path(graph_vec, start_intersection_id, end_intersection_id, path))
        print_path(path, start_place, dest_place);
    else
        cout << "No path found between these locations.\n";

    destroy_hash_table(place_table);
    return 0;
}

int find_nearest_intersection(double latitude, double longitude, const vector<intersection>& graph_vec)
{
    if (graph_vec.empty()) return -1;

    int nearest = 0;
    double min_distance = calculate_distance(latitude, longitude,
        graph_vec[0].latitude, graph_vec[0].longitude);

    for (size_t i = 1; i < graph_vec.size(); i++)
    {
        double dist = calculate_distance(latitude, longitude,
            graph_vec[i].latitude, graph_vec[i].longitude);
        if (dist < min_distance)
        {
            min_distance = dist;
            nearest = static_cast<int>(i);
        }
    }
    return nearest;
}
