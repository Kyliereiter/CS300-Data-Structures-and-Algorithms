// ProjectTwo.cpp
// ABCU Course Planner, CS 300 Project Two
// Single-file C++ program using a Binary Search Tree

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>

#ifdef _WIN32
#include <direct.h>   // _getcwd
#define GETCWD _getcwd
#define PATH_SEP "\\"
#else
#include <unistd.h>
#define GETCWD getcwd
#define PATH_SEP "/"
#endif

using namespace std;

// ----------------------------- Data model -----------------------------
struct Course {
    string number;
    string title;
    vector<string> prerequisites;
};

struct Node {
    Course data;
    Node* left;
    Node* right;
    explicit Node(const Course& c) : data(c), left(nullptr), right(nullptr) {}
};

class BST {
public:
    BST() : root(nullptr), count(0) {}
    ~BST() { destroy(root); }

    // No copy or move operations needed if we never move trees
    BST(const BST&) = delete;
    BST& operator=(const BST&) = delete;

    void clear() { destroy(root); root = nullptr; count = 0; }

    void insertOrUpdate(const Course& c) { root = insert(root, c); }

    const Course* find(const string& number) const {
        Node* cur = root;
        while (cur) {
            if (number == cur->data.number) return &cur->data;
            cur = (number < cur->data.number) ? cur->left : cur->right;
        }
        return nullptr;
    }

    template <typename F>
    void inOrder(F fn) const { traverse(root, fn); }

private:
    Node* root;
    size_t count;

    static void destroy(Node* n) {
        if (!n) return;
        destroy(n->left);
        destroy(n->right);
        delete n;
    }

    Node* insert(Node* n, const Course& c) {
        if (!n) { ++count; return new Node(c); }
        if (c.number == n->data.number) {
            n->data = c;                         // update existing
        }
        else if (c.number < n->data.number) {
            n->left = insert(n->left, c);
        }
        else {
            n->right = insert(n->right, c);
        }
        return n;
    }

    template <typename F>
    static void traverse(Node* n, F fn) {
        if (!n) return;
        traverse(n->left, fn);
        fn(n->data);
        traverse(n->right, fn);
    }
};

// ----------------------------- Helpers -----------------------------
static inline string trim(string s) {
    s.erase(s.begin(), find_if(s.begin(), s.end(), [](int ch) { return !isspace(ch); }));
    s.erase(find_if(s.rbegin(), s.rend(), [](int ch) { return !isspace(ch); }).base(), s.end());
    return s;
}

static inline string toUpper(string s) {
    transform(s.begin(), s.end(), s.begin(), ::toupper);
    return s;
}

static inline string stripQuotes(const string& s) {
    if (s.size() >= 2 &&
        ((s.front() == '"' && s.back() == '"') ||
            (s.front() == '\'' && s.back() == '\''))) {
        return s.substr(1, s.size() - 2);
    }
    return s;
}

static vector<string> splitCSV(const string& line) {
    vector<string> fields;
    string field;
    stringstream ss(line);
    while (getline(ss, field, ',')) fields.push_back(trim(field));
    return fields;
}

static string getCwdString() {
    char buf[4096] = { 0 };
    if (GETCWD(buf, sizeof(buf) - 1)) return string(buf);
    return string(".");
}

static bool isAbsolutePath(const string& p) {
#ifdef _WIN32
    return p.size() > 2 && isalpha((unsigned char)p[0]) && p[1] == ':'; // C:\...
#else
    return !p.empty() && p[0] == '/';
#endif
}

// Try several filename candidates, so users can type with or without extension
static ifstream tryOpenInput(const string& userInput) {
    const string requested = stripQuotes(trim(userInput));
    const bool hasExt = requested.find_last_of('.') != string::npos;
    const string cwd = getCwdString();

    vector<string> candidates;
    // as typed, relative and absolute
    candidates.push_back(requested);
    candidates.push_back(cwd + string(PATH_SEP) + requested);

    if (!hasExt) {
        // also try .txt and .csv
        candidates.push_back(requested + ".txt");
        candidates.push_back(cwd + string(PATH_SEP) + requested + ".txt");
        candidates.push_back(requested + ".csv");
        candidates.push_back(cwd + string(PATH_SEP) + requested + ".csv");
    }

    ifstream in;
    for (const auto& c : candidates) {
        in.clear();
        in.open(c.c_str());
        if (in) return in;
    }
    return ifstream(); // empty stream
}

// ----------------------------- Load logic -----------------------------
// Load directly into 'bst' to avoid any move/copy of a tree.
static size_t loadCourses(const string& userPath, BST& bst) {
    ifstream in = tryOpenInput(userPath);
    if (!in) {
        ostringstream msg;
        msg << "Could not open file. Current working directory is "
            << getCwdString()
            << ". Place the file there or enter a full path.";
        throw runtime_error(msg.str());
    }

    // Fresh load each time
    bst.clear();

    string line;
    size_t loaded = 0;
    while (getline(in, line)) {
        line = trim(line);
        if (line.empty()) continue;

        vector<string> t = splitCSV(line);
        if (t.size() < 2) continue;

        Course c;
        c.number = toUpper(t[0]);
        c.title = t[1];
        for (size_t i = 2; i < t.size(); ++i)
            if (!t[i].empty()) c.prerequisites.push_back(toUpper(t[i]));

        bst.insertOrUpdate(c);
        ++loaded;
    }
    return loaded;
}

// ----------------------------- Printing -----------------------------
static void printCourseList(const BST& bst) {
    cout << "Here is a sample schedule:\n" << endl;
    bst.inOrder([](const Course& c) {
        cout << c.number << ", " << c.title << "\n" << endl;
        });
}

static void printCourseInfo(const BST& bst, const string& courseNum) {
    string key = toUpper(trim(courseNum));
    const Course* course = bst.find(key);
    if (!course) { cout << "Course not found." << endl << endl; return; }

    cout << course->number << ", " << course->title << "\n" << endl;

    if (course->prerequisites.empty()) {
        cout << "No prerequisites\n" << endl;
        return;
    }
    cout << "Prerequisites: ";
    for (size_t i = 0; i < course->prerequisites.size(); ++i) {
        if (i) cout << ", ";
        cout << course->prerequisites[i];
    }
    cout << "\n" << endl;
}

static void printMenu() {
    cout << "1. Load Data Structure." << endl;
    cout << "2. Print Course List." << endl;
    cout << "3. Print Course." << endl;
    cout << "9. Exit" << endl << endl;
}

// ----------------------------- Main -----------------------------
int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    cout << "Welcome to the course planner.\n" << endl;

    BST bst;
    bool dataLoaded = false;

    while (true) {
        printMenu();
        cout << "What would you like to do? ";

        string choice;
        if (!getline(cin, choice)) break;
        choice = trim(choice);
        cout << endl;

        if (choice == "1") {
            cout << "Enter the file name: ";
            string filename;
            if (!getline(cin, filename)) { cout << "Input error.\n" << endl; continue; }

            try {
                size_t count = loadCourses(filename, bst); // load directly into bst
                dataLoaded = (count > 0);
                cout << "Loaded " << count << " course rows.\n" << endl;
            }
            catch (const exception& e) {
                cout << "Error: " << e.what() << "\n" << endl;
            }
        }
        else if (choice == "2") {
            if (!dataLoaded) {
                cout << "No data loaded. Choose option 1 first.\n" << endl;
            }
            else {
                printCourseList(bst);
            }
        }
        else if (choice == "3") {
            if (!dataLoaded) {
                cout << "No data loaded. Choose option 1 first.\n" << endl;
            }
            else {
                cout << "What course do you want to know about? ";
                string num; getline(cin, num);
                cout << endl;
                printCourseInfo(bst, num);
            }
        }
        else if (choice == "9") {
            cout << "Thank you for using the course planner!" << endl;
            break;
        }
        else {
            cout << choice << " is not a valid option.\n" << endl;
        }
    }
    return 0;
}
