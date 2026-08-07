#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <algorithm>
#include <map>
#include <set>
#include <iomanip>
#include <functional>
#include <windows.h>

using namespace std;


struct Person {        
    string uid;
    string full_name;
};




struct Circle {        
    string title;
    vector<string> participants;
};






namespace Json {

    struct Value {
        enum Type { Object, Array, String, Number, Null } type = Null;
        map<string, Value> obj;
        vector<Value> arr;
        string str;
        double num = 0.0;
    };

    struct Parser {
        const string& src;
        size_t pos = 0;

        Parser(const string& s) : src(s) {}

        void skipWhitespace() {
            while (pos < src.size() && isspace((unsigned char)src[pos]))
                ++pos;
        }

        Value parse_value() {
            skipWhitespace();
            if (pos >= src.size()) return {};
            char c = src[pos];
            if (c == '{') return parseObject();
            if (c == '[') return parseArray();
            if (c == '"') return parseString();
            if (c == '-' || isdigit(c)) return parseNumber();
            return {};
        }

        Value parseObject() {
            Value v;
            v.type = Value::Object;
            ++pos;                  
            skipWhitespace();
            if (pos < src.size() && src[pos] == '}') {
                ++pos;
                return v;
            }
            while (pos < src.size()) {
                skipWhitespace();
                Value key = parseString();
                skipWhitespace();
                if (pos < src.size() && src[pos] == ':')
                    ++pos;
                Value val = parse_value();
                v.obj[key.str] = val;
                skipWhitespace();
                if (pos < src.size() && src[pos] == ',') {
                    ++pos;
                    continue;
                }
                if (pos < src.size() && src[pos] == '}') {
                    ++pos;
                    break;
                }
                break;
            }
            return v;
        }

        Value parseArray() {
            Value v;
            v.type = Value::Array;
            ++pos;                    
            skipWhitespace();
            if (pos < src.size() && src[pos] == ']') {
                ++pos;
                return v;
            }
            while (pos < src.size()) {
                v.arr.push_back(parse_value());
                skipWhitespace();
                if (pos < src.size() && src[pos] == ',') {
                    ++pos;
                    continue;
                }
                if (pos < src.size() && src[pos] == ']') {
                    ++pos;
                    break;
                }
                break;
            }
            return v;
        }

        Value parseString() {
            Value v;
            v.type = Value::String;
            if (pos >= src.size() || src[pos] != '"')
                return v;
            ++pos;                   
            while (pos < src.size() && src[pos] != '"') {
                if (src[pos] == '\\' && pos + 1 < src.size()) {
                    ++pos;
                    char esc = src[pos];
                    switch (esc) {
                        case 'n':  v.str += '\n'; break;
                        case 't':  v.str += '\t'; break;
                        case 'r':  v.str += '\r'; break;
                        case '"':  v.str += '"';  break;
                        case '\\': v.str += '\\'; break;
                        default:   v.str += esc;  break;
                    }
                } else {
                    v.str += src[pos];
                }
                ++pos;
            }
            if (pos < src.size()) ++pos; 
            return v;
        }

        Value parseNumber() {
            Value v;
            v.type = Value::Number;
            size_t start = pos;
            while (pos < src.size() && (isdigit((unsigned char)src[pos]) ||
                   src[pos] == '-' || src[pos] == '.'))
                ++pos;
            try {
                v.num = stod(src.substr(start, pos - start));
            } catch (...) {
                v.num = 0.0;
            }
            return v;
        }
    };

    Value parse(const string& s) {
        return Parser(s).parse_value();
    }

} 









class FriendNet {
  private:
    unordered_map<string, Person> users;                    
    unordered_map<string, vector<pair<string, int>>> adj;   
    unordered_map<string, unordered_set<string>> adj_set;    
    unordered_map<string, Circle> circles;                  
    int total_edges = 0;                                     


    void run_bfs(
        const string& start,
        unordered_map<string, int>& dist,
        unordered_map<string, string>& parent
    ) const {

        dist.clear();
        parent.clear();
        queue<string> q;
        dist[start] = 0;
        parent[start] = "";
        q.push(start);

        while (!q.empty()) {

            string cur = q.front();
            q.pop();
            auto it = adj.find(cur);

            if (it == adj.end()) {
                continue;
            }

            for (const auto& neighbor : it->second) {

                const string& nb = neighbor.first;

                if (dist.find(nb) == dist.end()) {
                    dist[nb] = dist[cur] + 1;
                    parent[nb] = cur;
                    q.push(nb);
                }
            }
        }
    }







    bool user_exists(const string& id) const {
        return users.find(id) != users.end();
    }






    bool are_linked(
        const string& u, 
        const string& v
    ) const {
        auto it = adj_set.find(u);

        if (it == adj_set.end()) {
            return false;
        }

        return it->second.find(v) != it->second.end();
    }






    
    static string escape_json(const string& s) {
        string out;
        for (char c : s) {
            if (c == '"' || c == '\\') out += '\\';
            out += c;
        }

        return out;
    }







    void add_link_internal(
        const string& u, 
        const string& v, 
        int weight
    ) {
        adj[u].push_back({v, weight});
        adj[v].push_back({u, weight});
        adj_set[u].insert(v);
        adj_set[v].insert(u);
        ++total_edges;
    }

  public:


    bool load_from_file(const string& filename) {
        ifstream file(filename);

        if (!file.is_open()) {
            cerr << "[Error] File " << filename << " not found!" << endl;
            return false;
        }

        stringstream buffer;
        buffer << file.rdbuf();
        string content = buffer.str();
        file.close();

        clear_all();

        Json::Value root = Json::parse(content);

        if (root.type == Json::Value::Array) {
            for (auto& edge_val : root.arr) {

                if (edge_val.type != Json::Value::Object) {
                    continue;
                }

                string src_key = edge_val.obj.count("source") ? "source" : "u";
                string tgt_key = edge_val.obj.count("target") ? "target" : "v";
                string src     = edge_val.obj.count(src_key) ? edge_val.obj[src_key].str : "";
                string tgt     = edge_val.obj.count(tgt_key) ? edge_val.obj[tgt_key].str : "";
                int    weight  = edge_val.obj.count("weight") ? (int)edge_val.obj["weight"].num : 1;

                if (!src.empty() && !tgt.empty()) {
                    if (!user_exists(src)) {
                        users[src] = {src, "User " + src};
                        adj[src] = vector<pair<string, int>>();
                        adj_set[src] = unordered_set<string>();
                    }

                    if (!user_exists(tgt)) {
                        users[tgt] = {tgt, "User " + tgt};
                        adj[tgt] = vector<pair<string, int>>();
                        adj_set[tgt] = unordered_set<string>();
                    }

                    add_link_internal(src, tgt, weight);
                }
            }

            cout << "[OK] Loaded " << users.size() << " users, "
                << total_edges << " friendships, "
                << circles.size() << " groups." << endl;

            return true;
        }

        if (root.type == Json::Value::Object) {
          
            if (root.obj.count("users")) {
                for (auto& u_val : root.obj["users"].arr) {

                    string id = u_val.obj["id"].str;
                    string name = u_val.obj.count("name") ? u_val.obj["name"].str : ("User " + id);

                    if (!id.empty()) {
                        users[id] = {id, name};
                        adj[id] = vector<pair<string, int>>();
                        adj_set[id] = unordered_set<string>();
                    }
                }
            }

            string edge_key = root.obj.count("edges") ? "edges" : "links";

            if (root.obj.count(edge_key)) {
                for (auto& e_val : root.obj[edge_key].arr) {

                    string src_key = e_val.obj.count("source") ? "source" : "u";
                    string tgt_key = e_val.obj.count("target") ? "target" : "v";
                    string src     = e_val.obj[src_key].str;
                    string tgt     = e_val.obj[tgt_key].str;
                    int    weight  = e_val.obj.count("weight") ? (int)e_val.obj["weight"].num : 1;

                    if (!src.empty() && !tgt.empty()) {
                        if (!user_exists(src)) {
                            users[src] = {src, "User " + src};
                            adj[src] = vector<pair<string, int>>();
                            adj_set[src] = unordered_set<string>();
                        }

                        if (!user_exists(tgt)) {
                            users[tgt] = {tgt, "User " + tgt};
                            adj[tgt] = vector<pair<string, int>>();
                            adj_set[tgt] = unordered_set<string>();
                        }

                        add_link_internal(src, tgt, weight);
                    }
                }
            }

            
            if (root.obj.count("groups")) {
                for (auto& g_val : root.obj["groups"].arr) {

                    string gname = g_val.obj["name"].str;
                    vector<string> members;

                    if (g_val.obj.count("members")) {
                        for (auto& m_val : g_val.obj["members"].arr) {
                            members.push_back(m_val.str);
                        }
                    }

                    if (!gname.empty() && !members.empty()) {
                        circles[gname] = {gname, members};
                    }
                }
            }

            cout << "[OK] Loaded " << users.size() << " users, "
                 << total_edges << " friendships, "
                 << circles.size() << " groups." << endl;

            return true;
        }

        cerr << "[Error] Invalid JSON format." << endl;

        return false;
    }








    bool save_to_file(const string& filename) const {

        ofstream file(filename);

        if (!file.is_open()) {
            cerr << "[Error] Cannot create file!" << endl;
            return false;
        }

        file << "{\n  \"users\": [\n";
        bool first_user = true;

        for (auto it = users.begin(); it != users.end(); ++it) {

            if (!first_user) {
                file << ",\n";
            }

            file << "    {\"id\": \"" << escape_json(it->first) << "\", "
                << "\"name\": \"" << escape_json(it->second.full_name) << "\"}";

            first_user = false;
        }

        file << "\n  ],\n  \"edges\": [\n";

        first_user = true;
        for (auto it = adj.begin(); it != adj.end(); ++it) {
            for (const auto& neighbor : it->second) {

                if (it->first < neighbor.first) {

                    if (!first_user) {
                        file << ",\n";
                    }

                    file << "    {\"source\": \"" << escape_json(it->first) << "\", "
                        << "\"target\": \"" << escape_json(neighbor.first) << "\", "
                        << "\"weight\": " << neighbor.second << "}";

                    first_user = false;
                }
            }
        }

        file << "\n  ],\n  \"groups\": [\n";

        first_user = true;
        for (auto it = circles.begin(); it != circles.end(); ++it) {

            if (!first_user) {
                file << ",\n";
            }

            file << "    {\"name\": \"" << escape_json(it->first) << "\", \"members\": [";
            bool first_member = true;

            for (const string& m : it->second.participants) {
                if (!first_member) {
                    file << ", ";
                }

                file << "\"" << escape_json(m) << "\"";
                first_member = false;
            }

            file << "]}";
            first_user = false;
        }

        file << "\n  ]\n}\n";
        file.close();
        cout << "[OK] Saved to " << filename << endl;

        return true;
    }

    





    void clear_all() {
        users.clear();
        adj.clear();
        adj_set.clear();
        circles.clear();
        total_edges = 0;
    }








    bool add_person(
        const string& id, 
        const string& name
    ) {
        if (user_exists(id)) {
            cout << "[Warning] User " << id << " already exists." << endl;
            return false;
        }

        users[id]   = {id, name};
        adj[id]     = vector<pair<string, int>>();
        adj_set[id] = unordered_set<string>();

        cout << "[OK] User " << id << " (" << name << ") added." << endl;

        return true;
    }







    bool remove_person(const string& id) {

        if (!user_exists(id)) {
            cout << "[Error] User " << id << " not found!" << endl;

            return false;
        }

        for (auto it = adj.begin(); it != adj.end(); ++it) {

            if (it->first == id) {
                continue;
            }

            auto& neighbors = it->second;
            for (auto nit = neighbors.begin(); nit != neighbors.end(); ) {
                if (nit->first == id) {

                    nit = neighbors.erase(nit);
                    adj_set[it->first].erase(id);
                    total_edges--;
                } else {

                    nit++;
                }
            }
        }

        adj.erase(id);
        adj_set.erase(id);
        users.erase(id);

        
        vector<string> to_remove;
        for (auto it = circles.begin(); it != circles.end(); it++) {

            auto& members = it->second.participants;
            auto pos = find(members.begin(), members.end(), id);

            if (pos != members.end()) {
                members.erase(pos);
                if (members.size() < 2) {
                    to_remove.push_back(it->first);
                }
            }
        }

        for (const string& gname : to_remove) {
            circles.erase(gname);
        }

        cout << "[OK] User " << id << " removed." << endl;

        return true;
    }







    bool rename_person(
        const string& id, 
        const string& new_name
    ) {
        if (!user_exists(id)) {
            cout << "[Error] User " << id << " not found!" << endl;
            return false;
        }

        users[id].full_name = new_name;
        cout << "[OK] User " << id << " renamed to " << new_name << "." << endl;

        return true;
    }






    bool add_link(
        const string& u, 
        const string& v, 
        int weight = 1
    ) {
        if (!user_exists(u) || !user_exists(v)) {
            cout << "[Error] One or both users do not exist!" << endl;
            return false;
        }

        if (u == v) {
            cout << "[Error] A user cannot be friends with themselves!" << endl;
            return false;
        }

        if (are_linked(u, v)) {
            cout << "[Warning] Friendship already exists." << endl;
            return false;
        }

        add_link_internal(u, v, weight);
        cout << "[OK] Friendship between " << u << " and " << v << " created." << endl;

        return true;
    }







    bool remove_link(
        const string& u, 
        const string& v
    ) {
        if (!user_exists(u) || !user_exists(v)) {
            cout << "[Error] One or both users not found!" << endl;
            return false;
        }

        if (!are_linked(u, v)) {
            cout << "[Error] Friendship does not exist!" << endl;
            return false;
        }

        
        auto& u_list = adj[u];
        for (auto it = u_list.begin(); it != u_list.end(); ) {

            if (it->first == v) {
                it = u_list.erase(it);
            }
            else {
                it++;
            }
        }
        
        auto& v_list = adj[v];
        for (auto it = v_list.begin(); it != v_list.end(); ) {

            if (it->first == u) {
                it = v_list.erase(it);
            }
            else {
                it++;
            }
        }

        adj_set[u].erase(v);
        adj_set[v].erase(u);
        --total_edges;

        cout << "[OK] Friendship between " << u << " and " << v << " removed." << endl;

        return true;
    }

    






    void show_friends(const string& id) const {

        if (!user_exists(id)) {
            cout << "[Error] User " << id << " not found!" << endl;

            return;
        }

        auto it = adj.find(id);
        if (it == adj.end() || it->second.empty()) {

            cout << "User " << id << " has no friends." << endl;

            return;
        }

        cout << "Friends of " << id << " (" << users.at(id).full_name << "): "
            << it->second.size() << endl;

        cout << "----------------------------------------" << endl;

        for (const auto& p : it->second) {
            auto pit = users.find(p.first);
            cout << "  - " << p.first;

            if (pit != users.end()) {
                cout << " (" << pit->second.full_name << ")";
            }

            cout << " [Weight: " << p.second << "]" << endl;
        }
    }







    void check_connectiv_ity(
        const string& u,
        const string& v
    ) const {

        if (!user_exists(u) || !user_exists(v)) {
            cout << "[Error] One or both users not found!" << endl;
            return;
        }

        if (u == v) {
            cout << "Yes, a user is connected to themselves." << endl;
            return;
        }

        unordered_map<string, int>    dist;
        unordered_map<string, string> parent;

        run_bfs(u, dist, parent);
        if (dist.find(v) != dist.end()) {
            cout << "Yes, " << u << " and " << v << " are connected." << endl;
            cout << "Distance: " << dist.at(v) << " edges" << endl;
        } else {
            cout << "No, " << u << " and " << v << " are in separate groups." << endl;
        }
    }







    void find_path(
        const string& u, 
        const string& v
    ) const {

        if (!user_exists(u) || !user_exists(v)) {
            cout << "[Error] One or both users not found!" << endl;
            return;
        }

        if (u == v) {
            cout << "Source and destination are the same: " << u << endl;
            return;
        }

        unordered_map<string, int>    dist;
        unordered_map<string, string> parent;

        run_bfs(u, dist, parent);

        if (dist.find(v) == dist.end()) {
            cout << "No path exists between " << u << " and " << v << "." << endl;
            return;
        }

        vector<string> path;
        string cur = v;

        while (!cur.empty()) {
            path.push_back(cur);
            auto it = parent.find(cur);

            if (it == parent.end() || it->second.empty()) {
                break;
            }

            cur = it->second;
        }

        reverse(path.begin(), path.end());
        cout << "Shortest path from " << u << " to " << v << ":" << endl;
        cout << "Path length: " << dist.at(v) << " edges" << endl;
        cout << "----------------------------------------" << endl;

        for (size_t i = 0; i < path.size(); i++) {
            cout << path[i];
            if (i < path.size() - 1) {
                cout << " -> ";
            }
        }

        cout << endl;
    }








    void suggest_friends(const string& id) const {

        if (!user_exists(id)) {
            cout << "[Error] User " << id << " not found!" << endl;

            return;
        }

        auto it = adj_set.find(id);

        if (it == adj_set.end() || it->second.empty()) {
            cout << "User " << id << " has no friends for suggestions." << endl;
            return;
        }

        unordered_map<string, int> mutual_count;

        for (const string& freind_id : it->second) {
            auto fit = adj.find(freind_id);

            if (fit == adj.end()) {
                continue;
            }

            for (const auto& p : fit->second) {
                const string& candidate = p.first;

                if (candidate == id) {
                    continue;
                }

                if (adj_set.at(id).find(candidate) != adj_set.at(id).end()) {
                    continue;
                }

                ++mutual_count[candidate];
            }
        }

        if (mutual_count.empty()) {
            cout << "No suggestions for " << id << "." << endl;

            return;
        }

        vector<pair<string, int>> suggestions(mutual_count.begin(), mutual_count.end());
        sort(
            suggestions.begin(), suggestions.end(),
            [](const pair<string, int>& a, const pair<string, int>& b) {
                return a.second > b.second;
            }
        );

        cout << "Friend suggestions for " << id << " (" << users.at(id).full_name << "):" << endl;
        cout << "----------------------------------------" << endl;

        for (const auto& s : suggestions) {

            auto pit = users.find(s.first);
            cout << "  - " << s.first;

            if (pit != users.end()) {
                cout << " (" << pit->second.full_name << ")";
            }

            cout << " | Mutual friends: " << s.second << endl;
        }
    }







    void display_groups() const {
        
        if (users.empty()) {
            cout << "Network is empty." << endl;

            return;
        }

        
        unordered_set<string> visited;
        vector<vector<string>> components;

        for (const auto& kv : users) {
            const string& start = kv.first;

            if (visited.find(start) != visited.end()) {
                continue;
            }

            vector<string> component;
            queue<string> q;
            q.push(start);
            visited.insert(start);

            while (!q.empty()) {
                string cur = q.front(); q.pop();
                component.push_back(cur);
                auto it = adj.find(cur);

                if (it != adj.end()) {
                    for (const auto& p : it->second) {
                        if (visited.find(p.first) == visited.end()) {
                            visited.insert(p.first);
                            q.push(p.first);
                        }
                    }
                }
            }

            components.push_back(component);
        }

        sort(
            components.begin(), 
            components.end(),
            [](const vector<string>& a, const vector<string>& b) {
                return a.size() > b.size();
            }
        );

        cout << "Number of connected components (groups): " << components.size() << endl;
        cout << "----------------------------------------" << endl;

        for (size_t i = 0; i < components.size(); i++) {
            cout << "Group " << (i + 1) << " (" << components[i].size() << " members):" << endl;

            for (const string& member : components[i]) {
                auto pit = users.find(member);
                cout << "  - " << member;

                if (pit != users.end()) {
                    cout << " (" << pit->second.full_name << ")";
                }

                cout << endl;
            }

            if (i < components.size() - 1) {
                cout << "----------------------------------------" << endl;
            }
        }
    }








    bool add_circle(
        const string&         name, 
        const vector<string>& members
    ) {

        if (circles.find(name) != circles.end()) {
            cout << "[Error] Group " << name << " already exists!" << endl;
            return false;
        }

        if (members.size() < 2) {
            cout << "[Error] A group must have at least 2 members." << endl;
            return false;
        }

        for (const string& m : members) {
            if (!user_exists(m)) {
                cout << "[Error] User " << m << " does not exist!" << endl;
                return false;
            }
        }

        bool any_new_links = false;
        for (size_t i = 0; i < members.size(); i++) {
            for (size_t j = i + 1; j < members.size(); j++) {

                if (!are_linked(members[i], members[j])) {
                    add_link_internal(members[i], members[j], 1);
                    any_new_links = true;
                }
            }
        }

        circles[name] = {name, members};
        cout << "[OK] Group " << name << " created with " << members.size() << " members.";

        if (any_new_links) {
            cout << " New friendships were created.";
        }

        cout << endl;

        return true;
    }









    bool remove_circle(const string& name) {
        
        auto it = circles.find(name);
        if (it == circles.end()) {
            cout << "[Error] Group " << name << " not found!" << endl;
            return false;
        }

        circles.erase(it);
        cout << "[OK] Group " << name << " removed. Friendships remain intact." << endl;

        return true;
    }








    void show_circles() const {

        if (circles.empty()) {
            cout << "No defined groups." << endl;

            return;
        }

        cout << "Defined Groups (" << circles.size() << "):" << endl;
        cout << "----------------------------------------" << endl;

        for (const auto& kv : circles) {

            cout << "Group: " << kv.first << " (" << kv.second.participants.size() << " members)" << endl;
            
            for (const string& m : kv.second.participants) {
                auto pit = users.find(m);
                cout << "  - " << m;

                if (pit != users.end()) {
                    cout << " (" << pit->second.full_name << ")";
                }

                cout << endl;
            }

            cout << "----------------------------------------" << endl;
        }
    }






    void show_popular() const {

        if (users.empty()) {
            cout << "Network is empty." << endl;
            return;
        }

        int max_friends = -1;
        for (const auto& kv : adj) {
            max_friends = max(max_friends, (int)kv.second.size());
        }

        if (max_friends <= 0) {
            cout << "No friendships in the network." << endl;
            return;
        }

        cout << "Users with most friends (" << max_friends << " friends):" << endl;
        cout << "----------------------------------------" << endl;

        for (const auto& kv : adj) {
            if ((int)kv.second.size() == max_friends) {

                auto pit = users.find(kv.first);
                cout << "  - " << kv.first;

                if (pit != users.end()) {
                    cout << " (" << pit->second.full_name << ")";
                }

                cout << endl;
            }
        }
    }







    void show_mutual(const string& u, const string& v) const {
        if (!user_exists(u) || !user_exists(v)) {
            cout << "[Error] One or both users not found!" << endl;
            return;
        }

        if (u == v) {
            cout << "Mutual friends of a user with themselves are all their friends." << endl;
            show_friends(u);

            return;
        }

        auto u_it = adj_set.find(u);
        auto v_it = adj_set.find(v);

        if (u_it == adj_set.end() || v_it == adj_set.end()) {
            cout << "Mutual friends: 0" << endl;

            return;
        }

        const auto& u_friends = u_it->second;
        const auto& v_friends = v_it->second;

        vector<string> common;

        if (u_friends.size() > v_friends.size()) {

            for (const string& f : v_friends) {
                if (u_friends.find(f) != u_friends.end()) common.push_back(f);
            }
        } else {
            for (const string& f : u_friends) {
                if (v_friends.find(f) != v_friends.end()) common.push_back(f);
            }
        }

        cout << "Mutual friends of " << u << " and " << v << ": " << common.size() << endl;
        cout << "----------------------------------------" << endl;

        for (const string& f : common) {

            auto pit = users.find(f);
            cout << "  - " << f;

            if (pit != users.end()) {
                cout << " (" << pit->second.full_name << ")";
            }

            cout << endl;
        }
    }







    void showStats() const {
        cout << "========== Network Statistics ==========" << endl;
        cout << "a. Total users: " << users.size() << endl;
        cout << "b. Total friendships: " << total_edges << endl;

        if (users.empty()) {
            cout << "c. Average degree: 0" << endl;
            cout << "d. Number of friendship group: 0" << endl;
            cout << "e. Most connected user: -" << endl;
            return;
        }

        double avg_degree = (2.0 * total_edges) / users.size();
        cout << "c. Average degree: " << fixed << setprecision(2) << avg_degree << endl;

        unordered_set<string> visited;
        int largest_component = 0;

        for (const auto& kv : users) {
            if (visited.find(kv.first) != visited.end()) {
                continue;
            }

            int comp_size = 0;
            queue<string> q;
            q.push(kv.first);
            visited.insert(kv.first);

            while (!q.empty()) {
                string cur = q.front(); q.pop();
                comp_size++;

                auto it = adj.find(cur);
                if (it != adj.end()) {

                    for (const auto& p : it->second) {

                        if (visited.find(p.first) == visited.end()) {
                            visited.insert(p.first);
                            q.push(p.first);
                        }
                    }
                }
            }

            largest_component = max(largest_component, comp_size);
        }

        string most_connected;
        int max_degree = -1;

        for (const auto& kv : adj) {
            int deg = (int)kv.second.size();
            if (deg > max_degree) {
                max_degree = deg;
                most_connected = kv.first;
            }
        }

        cout << "d. Number of friendship group size: " << largest_component << endl;
        auto pit = users.find(most_connected);
        cout << "e. Most connected user: " << most_connected;
        if (pit != users.end()) {
            cout << " (" << pit->second.full_name << ") - " << max_degree << " friends";
        }

        cout << endl;
    }







    void show_distances(const string& id) const {
        if (!user_exists(id)) {
            cout << "[Error] User " << id << " not found!" << endl;
            return;
        }

        unordered_map<string, int> dist;
        unordered_map<string, string> parent;
        run_bfs(id, dist, parent);

        vector<pair<string, int>> results;
        for (const auto& kv : users) {
            auto dit = dist.find(kv.first);
            if (dit != dist.end()) {
                results.push_back({kv.first, dit->second});
            }
                
            else {
                results.push_back({kv.first, -1});
            }       
        }

        sort(
            results.begin(), 
            results.end(),
            [](const pair<string,int>& a, const pair<string,int>& b) {
                if (a.second == -1 && b.second == -1) return a.first < b.first;
                if (a.second == -1) return false;
                if (b.second == -1) return true;

                return a.second < b.second;
            }
        );

        cout << "Distances from " << id << " (" << users.at(id).full_name << "):" << endl;
        cout << "----------------------------------------" << endl;

        for (const auto& r : results) {

            auto pit = users.find(r.first);
            cout << "  " << r.first;

            if (pit != users.end()) {
                cout << " (" << pit->second.full_name << ")";
            }

            cout << " : ";
            if (r.second == -1) {
                cout << "INF (unreachable)";
            }
            else if (r.second == 0) {
                cout << "0 (self)";
            }
            else {
                cout << r.second << " edges";
            }

            cout << endl;
        }
    }







    void show_all() const {

        if (users.empty()) {
            cout << "No users in the network." << endl;
            return;
        }

        cout << "User list (" << users.size() << " users):" << endl;
        cout << "----------------------------------------" << endl;

        for (const auto& kv : users) {
            auto ait = adj.find(kv.first);
            int friends = (ait != adj.end()) ? (int)ait->second.size() : 0;
            cout << "  " << kv.first << " (" << kv.second.full_name << ") - " << friends << " friends" << endl;
        }
    }

    void findKeyUsers() const {

        if (users.empty()) {
            cout << "Network is empty." << endl;
            return;
        }

        
        unordered_map<string, int>    disc;
        unordered_map<string, int>    low;
        unordered_map<string, string> parent;
        unordered_set<string>         articulation_points;
        int                           timer = 0;

        function<void(const string&)> dfs = [&](const string& u) {
            int children = 0;
            disc[u] = low[u] = timer++;

            if (adj.find(u) != adj.end()) {
                for (const auto& p : adj.at(u)) {
                    const string& v = p.first;

                    if (disc.find(v) == disc.end()) {
                        children++;
                        parent[v] = u;
                        dfs(v);
                        low[u] = min(low[u], low[v]);

                        if (parent[u].empty() && children > 1) {
                            articulation_points.insert(u);
                        }

                        if (!parent[u].empty() && low[v] >= disc[u]) {
                            articulation_points.insert(u);
                        }

                    } else if (v != parent[u]) {
                        low[u] = min(low[u], disc[v]);
                    }
                }
            }
        };

        for (const auto& kv : users) {
            if (disc.find(kv.first) == disc.end()) {
                parent[kv.first] = "";
                dfs(kv.first);
            }
        }

        if (articulation_points.empty()) {
            cout << "No key/bridge users found. The network is highly interconnected." << endl;
        } else {
            cout << "Key Users / Bridge Users (Articulation Points):" << endl;
            cout << "----------------------------------------" << endl;

            for (const string& id : articulation_points) {
                auto pit = users.find(id);
                cout << "  - " << id;
                if (pit != users.end()) {
                    cout << " (" << pit->second.full_name << ")";
                }

                cout << endl;
            }
        }
    }







    void detect_communities() const {

        if (users.empty()) {
            cout << "Network is empty." << endl;
            return;
        }
        
        unordered_map<string, int> label;
        int idx = 0;

        for (const auto& kv : users) {
            label[kv.first] = idx++;
        }

        bool changed = true;
        int  iterations = 0;

        while (changed && iterations < 10) {
            changed = false;
            iterations++;

            for (const auto& kv : users) {
                const string& u = kv.first;
                if (adj.find(u) == adj.end() || adj.at(u).empty()) {
                    continue;
                }

                unordered_map<int, int> voteCount;
                for (const auto& p : adj.at(u)) {
                    ++voteCount[label[p.first]];
                }

                int bestCount = -1;
                int bestLabel = label[u];

                for (const auto& vc : voteCount) {
                    if (vc.second > bestCount) {
                        bestCount = vc.second;
                        bestLabel = vc.first;
                    }
                }

                if (bestLabel != label[u]) {
                    label[u] = bestLabel;
                    changed = true;
                }
            }
        }

        unordered_map<int, vector<string>> communities;
        for (const auto& kv : users) {
            communities[label[kv.first]].push_back(kv.first);
        }

        cout << "Advanced Community Detection (Label Propagation):" << endl;
        cout << "Found " << communities.size() << " dense communities." << endl;
        cout << "----------------------------------------" << endl;
        for (const auto& kv : communities) {
            cout << "Community " << kv.first + 1 << " (" << kv.second.size() << " members):" << endl;

            for (const string& member : kv.second) {
                auto pit = users.find(member);
                cout << "  - " << member;
                if (pit != users.end()) {
                    cout << " (" << pit->second.full_name << ")";
                }

                cout << endl;
            }

            cout << "----------------------------------------" << endl;
        }
    }








    void findBestSpreaders(int k) const {
        if (users.empty() || k <= 0) {
            cout << "Invalid input or empty network." << endl;
            return;
        }

        if (k > (int)users.size()) {
            k = (int)users.size();
        }

        unordered_set<string> selected;
        cout << "Finding top " << k << " best spreaders (Greedy Selection)..." << endl;

        for (int i = 0; i < k; i++) {
            string best_user;
            int max_reach = -1;

            for (const auto& kv : users) {
                const string& candidate = kv.first;
                if (selected.find(candidate) != selected.end()) {
                    continue;
                }

                
                unordered_set<string> visited = selected;
                queue<string> q;
                q.push(candidate);
                visited.insert(candidate);

                while (!q.empty()) {

                    string cur = q.front(); q.pop();

                    if (adj.find(cur) != adj.end()) {
                        for (const auto& p : adj.at(cur)) {

                            if (visited.find(p.first) == visited.end()) {
                                visited.insert(p.first);
                                q.push(p.first);
                            }
                        }
                    }
                }

                int reach = visited.size();

                if (reach > max_reach) {
                    max_reach = reach;
                    best_user = candidate;
                }
            }

            if (!best_user.empty()) {

                selected.insert(best_user);
                auto pit = users.find(best_user);
                cout << "  " << (i + 1) << ". " << best_user;

                if (pit != users.end()) {
                    cout << " (" << pit->second.full_name << ")";
                }

                cout << " - Estimated Reach: " << max_reach << " users" << endl;
            }
        }

        cout << "----------------------------------------" << endl;
        cout << "These users will maximize news spreading." << endl;
    }
};
