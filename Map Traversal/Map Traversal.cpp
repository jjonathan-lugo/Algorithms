#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>
#include <vector>

using namespace std;

struct connection 
{
    string name_road;
    string type_road;
    int A_start;
    int B_end;
    double length;
    string direction; //for N, S, E, W, NE

    connection() :
        name_road(""), type_road(""),
        A_start(0), B_end(0), length(0.0), direction("")
    {}

    connection(string name_r, string type_r, int start, int end, double len, string dir) :
        name_road(name_r), type_road(type_r), A_start(start), B_end(end), length(len), direction(dir)
    {}      
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

    intersection() : 
        intersection_number(0), longitude(0.0), latitude(0.0),
        distance_nearest(0.0), states_abv(""), named_places("") 
    {}

    intersection(int its_num, double lon, double lat, double dist_near, string s_abv, string named_p) :
        intersection_number(its_num), longitude(lon), latitude(lat), 
        distance_nearest(dist_near), states_abv(s_abv), named_places(named_p) 
    {}
};

void read_intersections(string filename, vector<intersection>& intersections)
{
    ifstream intersection_file(filename);
    string line;
    if (intersection_file.is_open())
    {
        int intersection_count = 0;
        while (getline(intersection_file, line))
        {
            int line_size = line.length();
            if (line_size > 0 && line[line_size - 1] == '\n')
            {
                line = line.substr(0, line_size - 1);
            }
            stringstream line_ss(line);
            double longitude;
            double latitude;
            double distance_nearest;
            string states_abv;
            string named_places;

            line_ss >> longitude >> latitude >> distance_nearest >> states_abv;
            getline(line_ss, named_places);
            if (!named_places.empty() && named_places[0] == ' ') //for trimming leading spaces
                named_places = named_places.substr(1);

            intersection new_intersection(intersection_count, longitude, latitude, distance_nearest, states_abv, named_places);
            intersections.push_back(new_intersection);
            intersection_count++;
        }
    }
    intersection_file.close();
}

string calc_direction(intersection& start, intersection& end)
{
    double lat = end.latitude - start.latitude;
    double lon = end.longitude - start.longitude;

    string direction = "";
    if (lat > 0)
        direction += "N";
    else if (lat < 0)
        direction += "S";

    if (lon > 0)
        direction += "E";
    else if (lon < 0)
        direction += "W";

    if (direction == "")
        direction = "Same";

    return direction;
}

void read_connections(string filename, vector<intersection>& intersections)
{
    ifstream connection_file(filename);
    string line;
    if (connection_file.is_open())
    {

        while (getline(connection_file, line))
        {
            int line_size = line.length();
            if (line_size > 0 && line[line_size - 1] == '\n')
            {
                line = line.substr(0, line_size - 1);
            }
            stringstream line_ss(line);
            string name_road;
            string type_road;
            int A_start;
            int B_end;
            double length;

            line_ss >> name_road >> type_road >> A_start >> B_end >> length;
            
            if (A_start >= 0 && A_start < intersections.size() && B_end >=0 && B_end < intersections.size()) //direct acess checking bounds
            {
                string dir = calc_direction(intersections[A_start], intersections[B_end]);
                connection new_connection(name_road, type_road, A_start, B_end, length, dir);
                intersections[A_start].leading_roads.push_back(new_connection);
            }
        }
    }
    connection_file.close();
}

void make_graph(vector<intersection>& intersections, string intersection_File, string connection_File) //created a node
{
    read_intersections(intersection_File, intersections); //creates the intersection w/ its information
    read_connections(connection_File, intersections); //add leading roads to intersection
}

int find_intersection_name(vector<intersection>& intersections, string name)
{
    for (int i = 0; i < intersections.size(); i++)
    {
        if (intersections[i].named_places == name)
            return i;      
    }
    return -1;
}

void display_intersection(intersection& current)
{
    cout << current.named_places << ": " << current.states_abv << '\n';
    cout << "Intersection: " << current.intersection_number << '\n';
    cout << "Latitude: " << current.latitude << '\n';
    cout << "Longitude: " << current.longitude << '\n\n';
    
    cout << "Road Leading Out: " << '\n';
    for (int i = 0; i < current.leading_roads.size(); i++)
    {
        cout << "Path " << i + 1 << ": " << current.leading_roads[i].name_road
            << "--> leads to intersection " << current.leading_roads[i].B_end
            << " (" << current.leading_roads[i].direction << ")" << '\n';
    }
}

int user_choice(int maxPath) //task 2
{
    cout << "Input Number or EXIT: ";
    string input;
    cin >> input;
    if (input == "EXIT")
        return -1; //flag to show EXIT is chosen

    int choice;
    try //for some reason if type lowercase "exit" it will quick program properly
    {
        choice = stoi(input); //stoi() - convert string to int
    }
    catch (...) 
    {
        return -2;
    }

    if (choice >= 1 && choice <= maxPath)
        return choice;
    return -2; //flag to show invalid input 
}

void exploring_graph(vector<intersection>& intersections, string start_pos)
{
    int current_index = find_intersection_name(intersections, start_pos);
    if (current_index == -1)
    {
        cout << "Location Not Found";
        return; //exit function
    }
    while (true)
    {
        display_intersection(intersections[current_index]);
        int user_Input = user_choice(intersections[current_index].leading_roads.size());
        if (user_Input == -1) //if user types EXIT
            break;
        else if (user_Input == -2) //type invalid input
            cout << "Invalid Input!";
        else //specifies taht it only runs for valid inputs || w/o it runs every time even with invalid inputs
            current_index = intersections[current_index].leading_roads[user_Input - 1].B_end;
    }
}

int main() 
{
    vector<intersection> graph;
    make_graph(graph, "intersections.txt", "connections.txt");
    
    cout << "Enter Start Location: ";
    string start_pos;
    getline(cin, start_pos);

    exploring_graph(graph, start_pos);

    cout << "Program Finished" << '\n';

    return 0;
}
