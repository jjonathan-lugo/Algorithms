#include <iostream>
#include <fstream>
#include <string>

using namespace std;

//Usually for full names we think of 1 person not more than 1
//usualy for place we think of 1 thing not as two sepereate things
//Convert " " -> "_" then from "_" -> " "
//EX) John Do ----> without = "John" "Do" || with = "John Do"
//EX) New York -> without = "New" "York" || with = "New York"
void replace_characters(string& str, char from, char to)
{
    for (int i = 0; i < (int)str.length(); ++i)
        if (str[i] == from) str[i] = to;
}

struct Person
{
    string lastName;
    string firstName;
    string state;
    string zipCode;
    string birthYear;
    string birthMonth;
    string birthDay;
    string password;
    string bankBalance;
    string ssn;

    Person() :
        lastName(""), firstName(""), state(""), zipCode(""),
        birthYear(""), birthMonth(""), birthDay(""),
        password(""), bankBalance(""), ssn("") {
    }

    Person(string ln, string fn, string st, string zip, string by, string bm, string bd, string pw, string bb, string s) :
        lastName(ln), firstName(fn), state(st), zipCode(zip),
        birthYear(by), birthMonth(bm), birthDay(bd),
        password(pw), bankBalance(bb), ssn(s) {
    }
};

struct TreeNode
{
    Person data;
    TreeNode* left;
    TreeNode* right;
    int size;

    TreeNode(Person p) :
        data(p),
        left(nullptr),
        right(nullptr),
        size(1) {
    }
};

//Moved outside of class - comparekeys not in tree function
int compare_keys(const Person& a, const Person& b)
{
    if (a.lastName < b.lastName) return -1;
    if (a.lastName > b.lastName) return 1;
    if (a.firstName < b.firstName) return -1;
    if (a.firstName > b.firstName) return 1;
    return 0;
}

//Moved outside class
bool older_than(const Person& a, const Person& b)
{
    if (a.birthYear != b.birthYear)
        return a.birthYear < b.birthYear;
    if (a.birthMonth != b.birthMonth)
        return a.birthMonth < b.birthMonth;
    return a.birthDay < b.birthDay;
}

class PersonTree
{
private:
    TreeNode* root;

    static int node_size(TreeNode* n)
    {
        return n ? n->size : 0;
    }

    static void update_size(TreeNode* n)
    {
        if (n) n->size = 1 + node_size(n->left) + node_size(n->right);
    }

    //Added helper functions to avoid repeated code, left/right decisions are made
    bool should_go_left(const string& last, const string& first, const Person& node_data)
    {
        return (last < node_data.lastName ||
            (last == node_data.lastName && first < node_data.firstName));
    }

    bool should_go_right(const string& last, const string& first, const Person& node_data)
    {
        return (last > node_data.lastName ||
            (last == node_data.lastName && first > node_data.firstName));
    }

    //Used const reference - treenode: remember to use pointers wherever appropriate
    TreeNode* insert(TreeNode* node, const Person& p)
    {
        if (!node)
            return new TreeNode(p);

        int cmp = compare_keys(p, node->data);
        if (cmp < 0)
            node->left = insert(node->left, p);
        else
            node->right = insert(node->right, p);

        update_size(node);
        return node;
    }

    void print_person(const Person& p)
    {
        cout << p.lastName << " " << p.firstName << " " << p.state << " "
            << p.zipCode << " " << p.birthYear << " " << p.birthMonth << " "
            << p.birthDay << " " << p.password << " "
            << p.bankBalance << " " << p.ssn << '\n';
    }

    //Kept only ppbtod function, removed duplicate inorder function
    void ppbtod(TreeNode* node)
    {
        if (!node)
            return;
        ppbtod(node->left);
        print_person(node->data);
        ppbtod(node->right);
    }

    TreeNode* search_with_parent(TreeNode* node, TreeNode*& parent, const string& last, const string& first)
    {
        parent = nullptr;
        TreeNode* current = node;

        while (current)
        {
            if (should_go_left(last, first, current->data))
            {
                parent = current;
                current = current->left;
            }
            else if (should_go_right(last, first, current->data))
            {
                parent = current;
                current = current->right;
            }
            else
            {
                return current;
            }
        }
        return nullptr;
    }

    //Used helper functions to avoid repeated left/right decision code
    TreeNode* search_exact(TreeNode* node, const string& last, const string& first)
    {
        if (!node)
            return nullptr;

        if (should_go_left(last, first, node->data))
            return search_exact(node->left, last, first);
        else if (should_go_right(last, first, node->data))
            return search_exact(node->right, last, first);
        else
            return node;
    }

    void family_last(TreeNode* node, const string& last)
    {
        if (!node)
            return;

        if (last < node->data.lastName)
            family_last(node->left, last);
        else if (last > node->data.lastName)
            family_last(node->right, last);
        else
        {
            family_last(node->left, last);
            print_person(node->data);
            family_last(node->right, last);
        }
    }

    void first_name_all(TreeNode* node, const string& first)
    {
        if (!node)
            return;
        first_name_all(node->left, first);
        if (node->data.firstName == first)
            print_person(node->data);
        first_name_all(node->right, first);
    }

    void oldest(TreeNode* node, bool& has, Person& best)
    {
        if (!node)
            return;
        oldest(node->left, has, best);
        if (!has || older_than(node->data, best))
        {
            best = node->data;
            has = true;
        }
        oldest(node->right, has, best);
    }

    TreeNode* min_node(TreeNode* node)
    {
        if (!node)
            return nullptr;
        while (node->left)
            node = node->left;
        return node;
    }

    //Used helper functions for consistent left/right decisions
    TreeNode* delete_by_name(TreeNode* node, const string& last, const string& first, bool& removed)
    {
        if (!node)
            return nullptr;

        if (should_go_left(last, first, node->data))
        {
            node->left = delete_by_name(node->left, last, first, removed);
        }
        else if (should_go_right(last, first, node->data))
        {
            node->right = delete_by_name(node->right, last, first, removed);
        }
        else
        {
            removed = true;

            if (!node->left && !node->right)
            {
                delete node;
                return nullptr;
            }
            else if (!node->left)
            {
                TreeNode* temp = node->right;
                delete node;
                return temp;
            }
            else if (!node->right)
            {
                TreeNode* temp = node->left;
                delete node;
                return temp;
            }
            else
            {
                TreeNode* successor = min_node(node->right);
                node->data = successor->data;
                node->right = delete_by_name(node->right, successor->data.lastName, successor->data.firstName, removed);
            }
        }

        update_size(node);
        return node;
    }

    TreeNode* kth(TreeNode* node, int k)
    {
        if (!node)
            return nullptr;

        int ls = node_size(node->left);

        if (k == ls + 1)
            return node;
        if (k <= ls)
            return kth(node->left, k);
        return kth(node->right, k - ls - 1);
    }

    void save_balanced_range(TreeNode* rootRef, int lo, int hi, ofstream& fout)
    {
        if (lo > hi)
            return;

        int mid = (lo + hi) / 2;
        TreeNode* m = kth(rootRef, mid);
        if (m)
        {
            string ln = m->data.lastName, fn = m->data.firstName, st = m->data.state,
                zip = m->data.zipCode, by = m->data.birthYear, bm = m->data.birthMonth,
                bd = m->data.birthDay, pw = m->data.password, bb = m->data.bankBalance,
                s = m->data.ssn;

            replace_characters(ln, ' ', '_'); replace_characters(fn, ' ', '_');
            replace_characters(st, ' ', '_'); replace_characters(zip, ' ', '_');
            replace_characters(by, ' ', '_'); replace_characters(bm, ' ', '_');
            replace_characters(bd, ' ', '_'); replace_characters(pw, ' ', '_');
            replace_characters(bb, ' ', '_'); replace_characters(s, ' ', '_');

            fout << ln << " " << fn << " " << st << " " << zip << " "
                << by << " " << bm << " " << bd << " "
                << pw << " " << bb << " " << s << '\n';
        }

        save_balanced_range(rootRef, lo, mid - 1, fout);
        save_balanced_range(rootRef, mid + 1, hi, fout);
    }

public:
    PersonTree() : root(nullptr) {}

    //Using const reference - remember to use pointers
    void insert(const Person& p)
    {
        root = insert(root, p);
    }

    //Changed replace_characters to pass by reference so it actually modifies the string
    void read_from_file(const string& filename)
    {
        ifstream infile(filename);
        if (!infile)
        {
            cout << "Error: Could not open file: " << filename << '\n';
            return;
        }

        string ln, fn, st, zip, by, bm, bd, pw, bb, s;
        while (infile >> ln >> fn >> st >> zip >> by >> bm >> bd >> pw >> bb >> s)
        {
            replace_characters(ln, '_', ' '); replace_characters(fn, '_', ' ');
            replace_characters(st, '_', ' '); replace_characters(zip, '_', ' ');
            replace_characters(by, '_', ' '); replace_characters(bm, '_', ' ');
            replace_characters(bd, '_', ' '); replace_characters(pw, '_', ' ');
            replace_characters(bb, '_', ' '); replace_characters(s, '_', ' ');

            insert(Person(ln, fn, st, zip, by, bm, bd, pw, bb, s));
        }
        infile.close();
        cout << "Finished reading file: " << filename << '\n';
    }

    void print_in_order()
    {
        ppbtod(root);
    }

    void find_and_print(const string& first, const string& last)
    {
        TreeNode* n = search_exact(root, last, first);
        if (n)
            print_person(n->data);
        else
            cout << "No such person: " << first << " " << last << '\n';
    }

    void family(const string& last)
    {
        family_last(root, last);
    }

    void first(const string& firstName)
    {
        first_name_all(root, firstName);
    }

    void oldest()
    {
        bool has = false;
        Person best;
        oldest(root, has, best);
        if (has)
            cout << best.firstName << " " << best.lastName << " " << best.zipCode << '\n';
        else
            cout << "No persons in database.\n";
    }

    void relocate(const string& first, const string& last, const string& newzip)
    {
        TreeNode* n = search_exact(root, last, first);
        if (n)
            n->data.zipCode = newzip;
        else
            cout << "No such person: " << first << " " << last << '\n';
    }

    //Deletion Testing: node addresses and tree size
    void remove(const string& first, const string& last)
    {
        cout << "Tree size before deletion: " << node_size(root) << '\n';

        TreeNode* target = search_exact(root, last, first);
        if (target)
        {
            cout << "Target: " << target << " " << target->data.firstName << " " << target->data.lastName << '\n';
            if (target->left) cout << "Left: " << target->left << " " << target->left->data.firstName << " " << target->left->data.lastName << '\n';
            if (target->right) cout << "Right: " << target->right << " " << target->right->data.firstName << " " << target->right->data.lastName << '\n';
        }

        bool removed = false;
        root = delete_by_name(root, last, first, removed);

        if (removed)
            cout << "Tree size after deletion: " << node_size(root) << '\n';
        else
            cout << "No such person: " << first << " " << last << '\n';
    }

    //Removed verification output
    void save_to_file(const string& filename)
    {
        ofstream fout(filename);
        if (!fout)
        {
            cout << "Error: Could not open file to save: " << filename << '\n';
            return;
        }

        int n = node_size(root);
        save_balanced_range(root, 1, n, fout);
        fout.close();
        cout << "Data saved to: " << filename << '\n';
    }

    void count_nodes()
    {
        cout << "Total nodes in tree: " << node_size(root) << '\n';
    }

    // Added testing function to find different node types for deletion testing
    void find_test_nodes()
    {
        cout << "Finding test nodes for deletion verification:\n";
        find_node_types(root, nullptr);
    }

private:
    void find_node_types(TreeNode* node, TreeNode* parent)
    {
        if (!node) return;

        bool is_leaf = (!node->left && !node->right);
        bool has_only_left = (node->left && !node->right);
        bool has_only_right = (!node->left && node->right);
        bool has_two_children = (node->left && node->right);
        bool is_root = (node == root);

        if (is_leaf)
            cout << "LEAF: " << node->data.firstName << " " << node->data.lastName << '\n';
        else if (has_only_left)
            cout << "ONLY LEFT CHILD: " << node->data.firstName << " " << node->data.lastName << '\n';
        else if (has_only_right)
            cout << "ONLY RIGHT CHILD: " << node->data.firstName << " " << node->data.lastName << '\n';
        else if (has_two_children)
            cout << "TWO CHILDREN: " << node->data.firstName << " " << node->data.lastName << '\n';

        if (is_root)
            cout << "ROOT: " << node->data.firstName << " " << node->data.lastName << '\n';

        find_node_types(node->left, node);
        find_node_types(node->right, node);
    }
};

int main(int argc, char* fln[])
{
    if (argc < 2)
    {
        cout << "Usage: " << fln[0] << " <file>\n";
        return 0;
    }

    PersonTree tree;
    tree.read_from_file(fln[1]);

    //Commented out unnecessary prompts in main loop
    //cout << "Enter a Command:FIND\nFAMILY\nFIRST\nPRINT\n";
    //cout << "OLDEST\nSAVE\nRELOCATE\nDELETE\nCOUNT\nTEST\nEXIT\n\n";

    string cm;
    while (cin >> cm)
    {
        if (cm == "FIND")
        {
            string first, last;
            cin >> first >> last;
            tree.find_and_print(first, last);
        }
        else if (cm == "FAMILY")
        {
            string last;
            cin >> last;
            tree.family(last);
        }
        else if (cm == "FIRST")
        {
            string first;
            cin >> first;
            tree.first(first);
        }
        else if (cm == "PRINT")
        {
            tree.print_in_order();
        }
        else if (cm == "OLDEST")
        {
            tree.oldest();
        }
        else if (cm == "SAVE")
        {
            string filename;
            cin >> filename;
            tree.save_to_file(filename);
        }
        else if (cm == "RELOCATE")
        {
            string first, last, newzip;
            cin >> first >> last >> newzip;
            tree.relocate(first, last, newzip);
        }
        else if (cm == "DELETE")
        {
            string first, last;
            cin >> first >> last;
            tree.remove(first, last);
        }
        else if (cm == "COUNT")
        {
            tree.count_nodes();
        }
        else if (cm == "TEST")
        {
            tree.find_test_nodes();
        }
        else if (cm == "EXIT")
        {
            break;
        }
    }

    return 0;
}
