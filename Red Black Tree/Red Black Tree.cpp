#include <iostream>
#include <cstdint>
#include <vector>
#include <string>

using namespace std;

template <typename T, typename K>
class two_three_tree
{
public:
    two_three_tree() : root(nullptr) {}
    ~two_three_tree() { delete_tree(root); }

    void insert(T item)
    {
        if (root == nullptr)
        {
            root = new node(item);
            return;
        }

        T promoted_item = T();
        bool did_split = false;
        node* new_root_value = insert_helper(get_pointer(root), item, promoted_item, did_split);

        if (did_split)
            root = new node(promoted_item, root, new_root_value);
    }

    T find(K key)
    {
        if (root == nullptr)
            return T();

        return find_helper(get_pointer(root), key);
    }

    bool remove(K key)
    {
        if (root == nullptr)
            return false;

        bool underflow_occurred = false;
        bool item_found = remove_helper(get_pointer(root), key, underflow_occurred);

        if (underflow_occurred)
            handle_root_underflow();

        return item_found;
    }

    void print_nicely(ostream& ostr)
    {
        if (root == nullptr)
        {
            ostr << "Tree is Empty\n";
            return;
        }

        print_helper(root, ostr, 0);
    }

private:
    struct node
    {
        T data1;
        T data2;
        node* child1;
        node* child2;
        node* child3;
        bool is_three_node;

        node(T data)
        {
            data1 = data;
            data2 = T();
            child1 = nullptr;
            child2 = nullptr;
            child3 = nullptr;
            is_three_node = false;
        }

        node(T data, node* left_child, node* right_child)
        {
            data1 = data;
            data2 = T();
            child1 = left_child;
            child2 = right_child;
            child3 = nullptr;
            is_three_node = false;
        }
    };

    node* root;

    // Pointer color encoding helpers
    node* set_color(node* ptr, bool is_red)
    {
        if (ptr == nullptr)
            return nullptr;

        uintptr_t int_ptr = (uintptr_t)ptr;
        if (is_red)
            int_ptr = int_ptr | 1;
        else
            int_ptr = int_ptr & ~1;

        return (node*)int_ptr;
    }

    bool get_color(node* ptr)
    {
        uintptr_t int_ptr = (uintptr_t)ptr;
        return (int_ptr & 1) != 0;
    }

    node* get_pointer(node* ptr)
    {
        uintptr_t int_ptr = (uintptr_t)ptr;
        int_ptr = int_ptr & ~1;
        return (node*)int_ptr;
    }

    bool is_red(node* ptr)
    {
        return get_color(ptr);
    }

    bool is_black(node* ptr)
    {
        return !get_color(ptr);
    }

    // Node property helpers
    K get_key(T item)
    {
        if (item == nullptr)
            return K();
        return item->key();
    }

    bool is_leaf(node* n)
    {
        return (get_pointer(n->child1) == nullptr &&
            get_pointer(n->child2) == nullptr &&
            get_pointer(n->child3) == nullptr);
    }

    int num_items(node* n)
    {
        return n->is_three_node ? 2 : 1;
    }

    // Cleanup
    void delete_tree(node* curr)
    {
        if (curr == nullptr)
            return;

        node* clean_curr = get_pointer(curr);
        delete_tree(clean_curr->child1);
        delete_tree(clean_curr->child2);
        delete_tree(clean_curr->child3);
        delete clean_curr;
    }

    // Helper to count total nodes at a level
    int count_nodes_at_level(node* curr, int target_level, int current_level)
    {
        if (curr == nullptr)
            return 0;

        node* clean_curr = get_pointer(curr);

        if (current_level == target_level)
            return 1;

        if (current_level < target_level)
        {
            int count = count_nodes_at_level(clean_curr->child1, target_level, current_level + 1);
            count += count_nodes_at_level(clean_curr->child2, target_level, current_level + 1);
            if (clean_curr->is_three_node)
                count += count_nodes_at_level(clean_curr->child3, target_level, current_level + 1);
            return count;
        }

        return 0;
    }

    // Helper to get tree height
    int get_height(node* curr)
    {
        if (curr == nullptr)
            return 0;

        node* clean_curr = get_pointer(curr);
        if (is_leaf(clean_curr))
            return 1;

        int h1 = get_height(clean_curr->child1);
        int h2 = get_height(clean_curr->child2);
        int h3 = clean_curr->is_three_node ? get_height(clean_curr->child3) : 0;

        return 1 + max(h1, max(h2, h3));
    }

    // Store nodes with their positions for a level
    void collect_nodes_at_level(node* curr, int target_level, int current_level,
        int& position, vector<pair<int, string>>& nodes)
    {
        if (curr == nullptr)
            return;

        node* clean_curr = get_pointer(curr);

        if (current_level == target_level)
        {
            string node_str;

            if (clean_curr->is_three_node)
            {
                // For 3-nodes, show as [val1|val2] with color indicators
                node_str = "[" + to_string(get_key(clean_curr->data1)) + "|" +
                    to_string(get_key(clean_curr->data2)) + "]";
                node_str += (is_red(curr) ? "(R)" : "(B)");
            }
            else
            {
                // For 2-nodes, show as [val] with color indicator
                node_str = "[" + to_string(get_key(clean_curr->data1)) + "]";
                node_str += (is_red(curr) ? "(R)" : "(B)");
            }

            nodes.push_back({ position, node_str });
            position++;
        }
        else if (current_level < target_level)
        {
            collect_nodes_at_level(clean_curr->child1, target_level, current_level + 1, position, nodes);
            collect_nodes_at_level(clean_curr->child2, target_level, current_level + 1, position, nodes);
            if (clean_curr->is_three_node)
                collect_nodes_at_level(clean_curr->child3, target_level, current_level + 1, position, nodes);
        }
    }

    // UPDATED PRINT FUNCTION - Better alignment with (R)/(B) labels
    void print_helper(node* curr, ostream& ostr, int level)
    {
        if (curr == nullptr)
            return;

        int height = get_height(curr);
        int max_nodes = 1;
        for (int i = 1; i < height; i++)
            max_nodes *= 3; // Maximum 3 children per node in 2-3 tree

        int node_width = 8; // Width allocated per node position

        for (int i = 0; i < height; i++)
        {
            vector<pair<int, string>> nodes;
            int position = 0;
            collect_nodes_at_level(curr, i, 0, position, nodes);

            // Calculate spacing
            int nodes_at_level = count_nodes_at_level(curr, i, 0);
            int total_width = node_width * (1 << (height - i - 1));

            // Print nodes with proper spacing
            int last_pos = 0;
            for (const auto& node_pair : nodes)
            {
                int node_position = node_pair.first;
                string node_str = node_pair.second;

                // Calculate center position for this node
                int center = total_width / 2 + node_position * total_width;
                int padding = center - last_pos - node_str.length() / 2;

                for (int j = 0; j < padding; j++)
                    ostr << " ";

                ostr << node_str;
                last_pos = center + node_str.length() / 2;
            }

            ostr << "\n";

            // Print branches if not last level
            if (i < height - 1)
            {
                last_pos = 0;
                int parent_idx = 0;
                for (const auto& node_pair : nodes)
                {
                    // Get the actual parent node to check if it has 3 children
                    node* parent_node = get_node_at_position(curr, i, 0, parent_idx);

                    // Check if parent is a 3-node AND has children (not a leaf)
                    bool has_three_children = false;
                    if (parent_node != nullptr)
                    {
                        node* clean_parent = get_pointer(parent_node);
                        has_three_children = (clean_parent->is_three_node &&
                            !is_leaf(clean_parent));
                    }

                    int node_position = node_pair.first;
                    int center = total_width / 2 + node_position * total_width;
                    int padding = center - last_pos - 1;

                    for (int j = 0; j < padding; j++)
                        ostr << " ";

                    if (has_three_children)
                    {
                        ostr << "/  |  \\";
                        last_pos = center + 6;
                    }
                    else
                    {
                        ostr << "/  \\";
                        last_pos = center + 3;
                    }

                    parent_idx++;
                }
                ostr << "\n";
            }
        }
    }

    // Helper to get node at specific position for branch drawing
    node* get_node_at_position(node* curr, int target_level, int current_level, int target_pos)
    {
        static int pos_counter;
        if (target_level == current_level && target_pos == 0)
            pos_counter = 0;

        if (curr == nullptr)
            return nullptr;

        node* clean_curr = get_pointer(curr);

        if (current_level == target_level)
        {
            if (pos_counter == target_pos)
                return curr;
            pos_counter++;
            return nullptr;
        }

        if (current_level < target_level)
        {
            node* result = get_node_at_position(clean_curr->child1, target_level, current_level + 1, target_pos);
            if (result != nullptr) return result;

            result = get_node_at_position(clean_curr->child2, target_level, current_level + 1, target_pos);
            if (result != nullptr) return result;

            if (clean_curr->is_three_node)
            {
                result = get_node_at_position(clean_curr->child3, target_level, current_level + 1, target_pos);
                if (result != nullptr) return result;
            }
        }

        return nullptr;
    }

    // Helper to collect parent node information
    void collect_parent_info(node* curr, int target_level, int current_level,
        int& position, vector<pair<int, bool>>& parent_info)
    {
        if (curr == nullptr)
            return;

        node* clean_curr = get_pointer(curr);

        if (current_level == target_level)
        {
            parent_info.push_back({ position, clean_curr->is_three_node });
            position++;
        }
        else if (current_level < target_level)
        {
            collect_parent_info(clean_curr->child1, target_level, current_level + 1, position, parent_info);
            collect_parent_info(clean_curr->child2, target_level, current_level + 1, position, parent_info);
            if (clean_curr->is_three_node)
                collect_parent_info(clean_curr->child3, target_level, current_level + 1, position, parent_info);
        }
    }

    // Insert helpers
    void sort_three_values(T item, T data1, T data2, T& val1, T& val2, T& val3)
    {
        if (get_key(item) < get_key(data1))
        {
            val1 = item;
            val2 = data1;
            val3 = data2;
        }
        else if (get_key(item) < get_key(data2))
        {
            val1 = data1;
            val2 = item;
            val3 = data2;
        }
        else
        {
            val1 = data1;
            val2 = data2;
            val3 = item;
        }
    }

    node* split_leaf(node* curr, T item, T& promoted, bool& did_split)
    {
        T val1, val2, val3;
        sort_three_values(item, curr->data1, curr->data2, val1, val2, val3);

        promoted = val2;
        did_split = true;

        curr->data1 = val1;
        curr->data2 = T();
        curr->child1 = set_color(curr->child1, false);
        curr->child2 = set_color(curr->child2, false);
        curr->child3 = nullptr;
        curr->is_three_node = false;

        node* new_right = new node(val3);
        return new_right;
    }

    node* insert_into_leaf(node* curr, T item)
    {
        if (get_key(item) < get_key(curr->data1))
        {
            curr->data2 = curr->data1;
            curr->data1 = item;
        }
        else
        {
            curr->data2 = item;
        }
        curr->is_three_node = true;
        curr->child2 = set_color(curr->child2, true);
        return curr;
    }

    node* find_target_child(node* curr, T item)
    {
        if (get_key(item) < get_key(curr->data1))
            return get_pointer(curr->child1);
        if (!curr->is_three_node)
            return get_pointer(curr->child2);
        if (get_key(item) < get_key(curr->data2))
            return get_pointer(curr->child2);
        return get_pointer(curr->child3);
    }

    node* insert_promoted_into_two_node(node* curr, T promoted_item, node* new_child)
    {
        if (get_key(promoted_item) < get_key(curr->data1))
        {
            curr->data2 = curr->data1;
            curr->child3 = curr->child2;
            curr->data1 = promoted_item;
            curr->child2 = new_child;
        }
        else
        {
            curr->data2 = promoted_item;
            curr->child3 = new_child;
        }

        curr->is_three_node = true;
        curr->child2 = set_color(curr->child2, true);
        return curr;
    }

    void determine_split_values(node* curr, T promoted_item, T& valA, T& valB, T& valC)
    {
        if (get_key(promoted_item) < get_key(curr->data1))
        {
            valA = promoted_item;
            valB = curr->data1;
            valC = curr->data2;
        }
        else if (get_key(promoted_item) < get_key(curr->data2))
        {
            valA = curr->data1;
            valB = promoted_item;
            valC = curr->data2;
        }
        else
        {
            valA = curr->data1;
            valB = curr->data2;
            valC = promoted_item;
        }
    }

    void determine_split_children(node* curr, T promoted_item, node* target_child, node* new_child,
        node*& childA, node*& childB, node*& childC, node*& childD)
    {
        if (get_key(promoted_item) < get_key(curr->data1))
        {
            childA = target_child;
            childB = new_child;
            childC = get_pointer(curr->child2);
            childD = get_pointer(curr->child3);
        }
        else if (get_key(promoted_item) < get_key(curr->data2))
        {
            childA = get_pointer(curr->child1);
            childB = target_child;
            childC = new_child;
            childD = get_pointer(curr->child3);
        }
        else
        {
            childA = get_pointer(curr->child1);
            childB = get_pointer(curr->child2);
            childC = target_child;
            childD = new_child;
        }
    }

    node* split_internal_node(node* curr, T promoted_item, node* target_child,
        node* new_child, T& promoted, bool& did_split)
    {
        T valA, valB, valC;
        node* childA, * childB, * childC, * childD;

        determine_split_values(curr, promoted_item, valA, valB, valC);
        determine_split_children(curr, promoted_item, target_child, new_child,
            childA, childB, childC, childD);

        promoted = valB;
        did_split = true;

        curr->data1 = valA;
        curr->data2 = T();
        curr->child1 = set_color(childA, false);
        curr->child2 = set_color(childB, false);
        curr->child3 = nullptr;
        curr->is_three_node = false;

        node* new_right = new node(valC, childC, childD);
        new_right->child1 = set_color(childC, false);
        new_right->child2 = set_color(childD, false);

        return new_right;
    }

    node* insert_helper(node* curr, T item, T& promoted, bool& did_split)
    {
        did_split = false;

        if (is_leaf(curr))
        {
            if (num_items(curr) == 1)
                return insert_into_leaf(curr, item);
            else
                return split_leaf(curr, item, promoted, did_split);
        }

        node* target_child = find_target_child(curr, item);
        T child_promoted_item = T();
        bool child_did_split = false;

        node* new_child_node = insert_helper(target_child, item, child_promoted_item, child_did_split);

        if (!child_did_split)
            return curr;

        if (num_items(curr) == 1)
            return insert_promoted_into_two_node(curr, child_promoted_item, new_child_node);
        else
            return split_internal_node(curr, child_promoted_item, target_child, new_child_node,
                promoted, did_split);
    }

    T find_helper(node* curr, K key)
    {
        node* clean_curr = get_pointer(curr);

        if (clean_curr == nullptr)
            return T();

        if (key == get_key(clean_curr->data1))
            return clean_curr->data1;

        if (clean_curr->is_three_node && key == get_key(clean_curr->data2))
            return clean_curr->data2;

        node* target_child = nullptr;

        if (key < get_key(clean_curr->data1))
            target_child = get_pointer(clean_curr->child1);
        else if (!clean_curr->is_three_node)
            target_child = get_pointer(clean_curr->child2);
        else if (key < get_key(clean_curr->data2))
            target_child = get_pointer(clean_curr->child2);
        else
            target_child = get_pointer(clean_curr->child3);

        return find_helper(target_child, key);
    }

    // Remove helpers
    void handle_root_underflow()
    {
        if (root == nullptr)
            return;

        node* clean_root = get_pointer(root);

        if (num_items(clean_root) == 0)
        {
            delete clean_root;
            root = nullptr;
        }
        else if (num_items(clean_root) == 1 && !is_leaf(clean_root))
        {
            node* old_root = clean_root;
            root = get_pointer(old_root->child1);
            delete old_root;
        }
    }

    bool can_borrow_from_sibling(node* sibling)
    {
        if (sibling == nullptr)
            return false;
        return get_pointer(sibling)->is_three_node;
    }

    node* find_predecessor_leaf(node* start)
    {
        node* pred = start;
        while (!is_leaf(pred))
        {
            if (num_items(pred) == 1)
                pred = get_pointer(pred->child2);
            else
                pred = get_pointer(pred->child3);
        }
        return pred;
    }

    node* find_successor_leaf(node* start)
    {
        node* succ = start;
        while (!is_leaf(succ))
            succ = get_pointer(succ->child1);
        return succ;
    }

    void swap_with_predecessor(node* curr, K& key)
    {
        node* pred = find_predecessor_leaf(get_pointer(curr->child1));
        T swapped = curr->data1;

        if (num_items(pred) == 1)
        {
            curr->data1 = pred->data1;
            pred->data1 = swapped;
        }
        else
        {
            curr->data1 = pred->data2;
            pred->data2 = swapped;
        }

        key = get_key(swapped);
    }

    void swap_with_successor(node* curr, K& key)
    {
        node* succ = find_successor_leaf(get_pointer(curr->child3));
        T swapped = curr->data2;
        curr->data2 = succ->data1;
        succ->data1 = swapped;
        key = get_key(succ->data1);
    }

    bool remove_from_leaf(node* leaf, K key, bool& underflow)
    {
        if (num_items(leaf) == 1)
        {
            leaf->data1 = T();
            underflow = true;
            return true;
        }

        if (get_key(leaf->data1) == key)
            leaf->data1 = leaf->data2;

        leaf->data2 = T();
        leaf->is_three_node = false;
        return true;
    }

    int find_child_index(node* parent, K key)
    {
        if (key < get_key(parent->data1))
            return 0;
        if (!parent->is_three_node)
            return 1;
        if (key < get_key(parent->data2))
            return 1;
        return 2;
    }

    node* get_child_by_index(node* parent, int index)
    {
        if (index == 0)
            return get_pointer(parent->child1);
        if (index == 1)
            return get_pointer(parent->child2);
        return get_pointer(parent->child3);
    }

    void borrow_from_right_sibling(node* parent, int child_idx)
    {
        node* child = get_child_by_index(parent, child_idx);
        node* sibling = get_pointer(parent->child2);

        child->data1 = parent->data1;
        child->is_three_node = true;

        parent->data1 = sibling->data1;

        sibling->data1 = sibling->data2;
        sibling->data2 = T();
        sibling->is_three_node = false;

        child->child2 = get_pointer(sibling->child1);
        sibling->child1 = get_pointer(sibling->child2);
        sibling->child2 = get_pointer(sibling->child3);
        sibling->child3 = nullptr;
    }

    void borrow_from_left_sibling(node* parent, int child_idx)
    {
        node* child = get_child_by_index(parent, child_idx);
        node* sibling = get_pointer(parent->child1);

        child->data2 = parent->data1;
        child->is_three_node = true;

        parent->data1 = sibling->data2;

        sibling->data2 = T();
        sibling->is_three_node = false;

        child->child1 = get_pointer(sibling->child3);
        sibling->child3 = nullptr;
    }

    void merge_with_sibling(node* parent, int child_idx, bool& parent_underflow)
    {
        node* left_child = get_pointer(parent->child1);
        node* right_child = get_pointer(parent->child2);

        left_child->data2 = right_child->data1;
        left_child->data1 = parent->data1;
        left_child->is_three_node = true;

        left_child->child3 = get_pointer(right_child->child2);
        left_child->child2 = get_pointer(right_child->child1);

        delete right_child;

        parent->data1 = parent->data2;
        parent->data2 = T();
        parent->child2 = get_pointer(parent->child3);
        parent->child3 = nullptr;

        if (parent->is_three_node)
            parent->is_three_node = false;
        else
            parent_underflow = true;
    }

    void handle_underflow_at_child(node* parent, int child_idx, bool& underflow)
    {
        node* sibling = nullptr;
        bool can_borrow = false;

        if (child_idx == 0)
        {
            sibling = get_pointer(parent->child2);
            can_borrow = can_borrow_from_sibling(parent->child2);

            if (can_borrow)
                borrow_from_right_sibling(parent, child_idx);
            else
                merge_with_sibling(parent, child_idx, underflow);
        }
        else
        {
            sibling = get_pointer(parent->child1);
            can_borrow = can_borrow_from_sibling(parent->child1);

            if (can_borrow)
                borrow_from_left_sibling(parent, child_idx);
            else
                merge_with_sibling(parent, child_idx, underflow);
        }
    }

    bool remove_helper(node* curr, K key, bool& underflow)
    {
        node* clean_curr = get_pointer(curr);
        underflow = false;

        if (clean_curr == nullptr)
            return false;

        bool found_in_curr = false;
        int data_idx = -1;

        if (key == get_key(clean_curr->data1))
        {
            found_in_curr = true;
            data_idx = 0;
        }
        else if (clean_curr->is_three_node && key == get_key(clean_curr->data2))
        {
            found_in_curr = true;
            data_idx = 1;
        }

        if (found_in_curr)
        {
            if (is_leaf(clean_curr))
                return remove_from_leaf(clean_curr, key, underflow);

            if (data_idx == 0)
                swap_with_predecessor(clean_curr, key);
            else
                swap_with_successor(clean_curr, key);

            int child_idx = (data_idx == 0) ? 0 : 2;
            node* target = get_child_by_index(clean_curr, child_idx);

            bool result = remove_helper(target, key, underflow);

            if (underflow)
                handle_underflow_at_child(clean_curr, child_idx, underflow);

            return result;
        }

        int child_idx = find_child_index(clean_curr, key);
        node* target = get_child_by_index(clean_curr, child_idx);

        bool result = remove_helper(target, key, underflow);

        if (!result)
            return false;

        if (underflow)
            handle_underflow_at_child(clean_curr, child_idx, underflow);

        return true;
    }
};

class TestData
{
public:
    int value;

    TestData() : value(0) {}
    TestData(int v) : value(v) {}

    int key() const { return value; }
};

int main()
{
    // Test Case 1: Only BLACK nodes
    cout << "TEST 1: Tree with Only BLACK Nodes\n";
    cout << "Inserting: 50, 25, 75\n\n";
    {
        two_three_tree<TestData*, int> tree;
        tree.insert(new TestData(50));
        tree.insert(new TestData(25));
        tree.insert(new TestData(75));

        tree.print_nicely(cout);

        cout << "\nFind 25: ";
        TestData* found = tree.find(25);
        cout << (found && found->value == 25 ? "FOUND" : "NOT FOUND") << "\n";

        cout << "Find 100: ";
        found = tree.find(100);
        cout << (found == nullptr ? "NOT FOUND" : "FOUND") << "\n";

        cout << "\nDelete 25\n";
        tree.remove(25);
        cout << "Tree after deletion:\n";
        tree.print_nicely(cout);
        cout << "\n\n";
    }

    // Test Case 2: Larger tree
    cout << "TEST 2: Larger Tree with Only BLACK Nodes\n";
    cout << "Inserting: 40, 20, 60, 10, 30, 50, 70\n\n";
    {
        two_three_tree<TestData*, int> tree;
        tree.insert(new TestData(40));
        tree.insert(new TestData(20));
        tree.insert(new TestData(60));
        tree.insert(new TestData(10));
        tree.insert(new TestData(30));
        tree.insert(new TestData(50));
        tree.insert(new TestData(70));

        tree.print_nicely(cout);

        cout << "\nFind 30: ";
        TestData* found = tree.find(30);
        cout << (found && found->value == 30 ? "FOUND" : "NOT FOUND") << "\n";

        cout << "Find 60: ";
        found = tree.find(60);
        cout << (found && found->value == 60 ? "FOUND" : "NOT FOUND") << "\n";

        cout << "\nDelete 20\n";
        tree.remove(20);
        cout << "Tree after deletion:\n";
        tree.print_nicely(cout);

        cout << "\nFind 20 (deleted): ";
        found = tree.find(20);
        cout << (found == nullptr ? "NOT FOUND" : "FOUND") << "\n\n";
    }

    // Test Case 3: Tree with 3-nodes (shows [a|b] format)
    cout << "TEST 3: Tree with 3-nodes\n";
    cout << "Inserting: 30, 20, 40, 10\n\n";
    {
        two_three_tree<TestData*, int> tree;
        tree.insert(new TestData(30));
        tree.insert(new TestData(20));
        tree.insert(new TestData(40));
        tree.insert(new TestData(10));

        tree.print_nicely(cout);

        cout << "\nFind 10: ";
        TestData* found = tree.find(10);
        cout << (found && found->value == 10 ? "FOUND" : "NOT FOUND") << "\n";

        cout << "Find 40: ";
        found = tree.find(40);
        cout << (found && found->value == 40 ? "FOUND" : "NOT FOUND") << "\n";

        cout << "Find 99: ";
        found = tree.find(99);
        cout << (found == nullptr ? "NOT FOUND" : "FOUND") << "\n";

        cout << "\nDelete 40\n";
        tree.remove(40);
        cout << "Tree after deletion:\n";
        tree.print_nicely(cout);
        cout << "\n\n";
    }

    // Test Case 4: Multiple 3-nodes
    cout << "TEST 4: Larger Tree with Multiple 3-nodes\n";
    cout << "Inserting: 50, 30, 70, 20, 40, 60, 80, 10\n\n";
    {
        two_three_tree<TestData*, int> tree;
        tree.insert(new TestData(50));
        tree.insert(new TestData(30));
        tree.insert(new TestData(70));
        tree.insert(new TestData(20));
        tree.insert(new TestData(40));
        tree.insert(new TestData(60));
        tree.insert(new TestData(80));
        tree.insert(new TestData(10));

        tree.print_nicely(cout);

        cout << "\nFind 10: ";
        TestData* found = tree.find(10);
        cout << (found && found->value == 10 ? "FOUND" : "NOT FOUND") << "\n";

        cout << "Find 50: ";
        found = tree.find(50);
        cout << (found && found->value == 50 ? "FOUND" : "NOT FOUND") << "\n";

        cout << "Find 80: ";
        found = tree.find(80);
        cout << (found && found->value == 80 ? "FOUND" : "NOT FOUND") << "\n";

        cout << "\nDelete 30\n";
        tree.remove(30);
        cout << "Tree after deleting 30:\n";
        tree.print_nicely(cout);

        cout << "\nDelete 70\n";
        tree.remove(70);
        cout << "Tree after deleting 70:\n";
        tree.print_nicely(cout);

        cout << "\nFind 30 (deleted): ";
        found = tree.find(30);
        cout << (found == nullptr ? "NOT FOUND" : "FOUND") << "\n";

        cout << "Find 70 (deleted): ";
        found = tree.find(70);
        cout << (found == nullptr ? "NOT FOUND" : "FOUND") << "\n\n";
    }

    // Test Case 5: Tree showing 3-node with pipe separator clearly
    cout << "TEST 5: Tree with Clear 3-node Examples\n";
    cout << "Inserting: 12, 10, 14, 7, 8, 9, 11, 13, 15\n\n";
    {
        two_three_tree<TestData*, int> tree;
        tree.insert(new TestData(12));
        tree.insert(new TestData(10));
        tree.insert(new TestData(14));
        tree.insert(new TestData(7));
        tree.insert(new TestData(8));
        tree.insert(new TestData(9));
        tree.insert(new TestData(11));
        tree.insert(new TestData(13));
        tree.insert(new TestData(15));

        tree.print_nicely(cout);

        cout << "\nFind 8: ";
        TestData* found = tree.find(8);
        cout << (found && found->value == 8 ? "FOUND" : "NOT FOUND") << "\n";

        cout << "Find 13: ";
        found = tree.find(13);
        cout << (found && found->value == 13 ? "FOUND" : "NOT FOUND") << "\n\n";
    }

    // Test Case 6: Sequential insertion showing 3-node formation
    cout << "TEST 6: Sequential Insertion (Forms 3-nodes)\n";
    cout << "Inserting: 1, 2, 3, 4, 5, 6, 7\n\n";
    {
        two_three_tree<TestData*, int> tree;
        tree.insert(new TestData(1));
        tree.insert(new TestData(2));
        tree.insert(new TestData(3));
        tree.insert(new TestData(4));
        tree.insert(new TestData(5));
        tree.insert(new TestData(6));
        tree.insert(new TestData(7));

        tree.print_nicely(cout);

        cout << "\nDelete 4\n";
        tree.remove(4);
        cout << "Tree after deletion:\n";
        tree.print_nicely(cout);
        cout << "\n\n";
    }
    return 0;
}
