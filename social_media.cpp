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



class FriendNet {      
  private:

    unordered_map<string, Person> folks;              
    unordered_map<string, vector<string>> adj;        
    unordered_map<string, unordered_set<string>> adj_set;
    unordered_map<string, Circle> clubs;             
    int total_links = 0;                               



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

            string cur = q.front(); q.pop();
            auto it = adj.find(cur);
            
            if (it == adj.end()) {
                continue;
            } 

                for (const string& nb : it->second) {
                    if (dist.find(nb) == dist.end()) {
                    dist[nb] = dist[cur] + 1;
                    parent[nb] = cur;
                    q.push(nb);
                }
            }
        }
    }








    bool is_existing(const string& id) const {
        return folks.find(id) != folks.end();
    }








    bool is_linked(
        const string& u, 
        const string& v
    ) const {
        auto it = adj_set.find(u);

        if (it == adj_set.end()) {
            return false;
        }

        return it->second.find(v) != it->second.end();
    }






    void find_cliques(
        unordered_set<string>& current,
        unordered_set<string>& pending,
        unordered_set<string>& processed,
        vector<vector<string>>& output
    ) const {
        if (pending.empty() && processed.empty()) {
            if (!current.empty()) {
                output.push_back(vector<string>(current.begin(), current.end()));
            }

            return;
        }

        string pivot;
        int best_score = -1;
        unordered_set<string> union_set = pending;
        union_set.insert(processed.begin(), processed.end());

        for (const string& node : union_set) {
            int score = 0;
            auto it   = adj_set.find(node);

            if (it != adj_set.end()) {
                for (const string& v : pending) {
                    if (it->second.find(v) != it->second.end()) score++;
                }
            }

            if (score > best_score) {
                best_score = score;
                pivot = node;
            }
        }

        unordered_set<string> candidates;
        if (pivot.empty()) {
            candidates = pending;
        }
        else {
            auto pit = adj_set.find(pivot);
            for (const string& v : pending) {
                if (pit == adj_set.end() || pit->second.find(v) == pit->second.end()) {
                    candidates.insert(v);
                }
            }
        }

        vector<string> cand_list(candidates.begin(), candidates.end());
        for (const string& v : cand_list) {
            unordered_set<string> new_curr = current;
            new_curr.insert(v);

            unordered_set<string> new_pending;
            unordered_set<string> new_processed;
            auto vit = adj_set.find(v);

            if (vit != adj_set.end()) {

                for (const string& u : pending) {
                    if (vit->second.find(u) != vit->second.end()) new_pending.insert(u);
                }

                for (const string& u : processed) {
                    if (vit->second.find(u) != vit->second.end()) new_processed.insert(u);
                }
            }

            find_cliques(new_curr, new_pending, new_processed, output);

            pending.erase(v);
            processed.insert(v);
        }
    }






    static string get_field(
        const string& obj, 
        const string& key
    ) {
        string pattern = "\"" + key + "\"";
        size_t pos = obj.find(pattern);

        if (pos == string::npos) {
            return "";
        }

        pos = obj.find(':', pos);

        if (pos == string::npos) {
            return "";
        }

        pos = obj.find('"', pos);

        if (pos == string::npos) {
            return "";
        }

        size_t end = obj.find('"', pos + 1);

        if (end == string::npos) {
            return "";
        }

        return obj.substr(pos + 1, end - pos - 1);
    }






    static int get_number(
        const string& obj, 
        const string& key
    ) {
        string pattern = "\"" + key + "\"";
        size_t pos = obj.find(pattern);

        if (pos == string::npos) {
            return 0;
        }

        pos = obj.find(':', pos);

        if (pos == string::npos) {
            return 0;
        }

        while (pos < obj.size() && (obj[pos] == ':' || isspace((unsigned char)obj[pos]))) {
            pos++;
        }

        int num = 0;

        while (pos < obj.size() && isdigit((unsigned char)obj[pos])) {
            num = num * 10 + (obj[pos] - '0');
            pos++;
        }

        return num;
    }







    static string escape_chars(const string& s) {
        string result;

        for (char c : s) {
            if (c == '\"' || c == '\\') result += '\\';
            result += c;
        }

        return result;
    }






    static vector<string> extract_list(
        const string& obj, 
        const string& key
    ) {
        vector<string> result;
        string pattern = "\"" + key + "\"";
        size_t pos = obj.find(pattern);

        if (pos == string::npos) {
            return result;
        }

        pos = obj.find('[', pos);

        if (pos == string::npos) {
            return result;
        }

        size_t end = obj.find(']', pos);

        if (end == string::npos) {
            return result;
        }

        string items = obj.substr(pos + 1, end - pos - 1);
        size_t i = 0;

        while (i < items.size()) {
            size_t q = items.find('"', i);

            if (q == string::npos) {
                break;
            }

            size_t eq = items.find('"', q + 1);

            if (eq == string::npos) {
                break;
            }

            result.push_back(items.substr(q + 1, eq - q - 1));

            i = eq + 1;
        }

        return result;
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

        size_t user_pos = content.find("\"users\"");
        if (user_pos != string::npos) {

            size_t arr_start = content.find('[', user_pos);
            size_t arr_end = content.find(']', arr_start);

            if (arr_start != string::npos && arr_end != string::npos) {
                string arr = content.substr(arr_start, arr_end - arr_start + 1);
                size_t pos = 0;

                while (pos < arr.size()) {
                    size_t obj_start = arr.find('{', pos);

                    if (obj_start == string::npos) {
                        break;
                    }

                    size_t obj_end = arr.find('}', obj_start);

                    if (obj_end == string::npos) {
                        break;
                    }

                    string obj = arr.substr(obj_start, obj_end - obj_start + 1);
                    string id = get_field(obj, "id");
                    string name = get_field(obj, "name");

                    if (!id.empty()) {
                        folks[id] = { id, name.empty() ? "User " + id : name };
                        adj[id] = vector<string>();
                        adj_set[id] = unordered_set<string>();
                    }

                    pos = obj_end + 1;
                }
            }
        }

        size_t edges_pos = content.find("\"edges\"");
        size_t edgesarr_start = content.find('[', edges_pos != string::npos ? edges_pos : 0);

        if (edgesarr_start == string::npos) edgesarr_start = content.find('[');

        if (edgesarr_start != string::npos) {
            size_t pos = edgesarr_start;

            while (pos < content.size()) {
                size_t obj_start = content.find('{', pos);

                if (obj_start == string::npos) {
                    break;
                }

                size_t obj_end = content.find('}', obj_start);

                if (obj_end == string::npos) {
                    break;
                }

                string obj = content.substr(obj_start, obj_end - obj_start + 1);
                string src = get_field(obj, "source");
                string tgt = get_field(obj, "target");

                if (!src.empty() && !tgt.empty()) {

                    if (!is_existing(src)) {
                        folks[src] = { src, "User " + src };
                        adj[src] = vector<string>();
                        adj_set[src] = unordered_set<string>();
                    }

                    if (!is_existing(tgt)) {
                        folks[tgt] = { tgt, "User " + tgt };
                        adj[tgt] = vector<string>();
                        adj_set[tgt] = unordered_set<string>();
                    }

                    add_link_internal(src, tgt);
                }

                pos = obj_end + 1;
            }
        }

        size_t group_pos = content.find("\"groups\"");
        if (group_pos != string::npos) {

            size_t arr_start = content.find('[', group_pos);
            size_t arr_end = content.find(']', arr_start);

            if (arr_start != string::npos && arr_end != string::npos) {
                string arr = content.substr(arr_start, arr_end - arr_start + 1);
                size_t pos = 0;

                while (pos < arr.size()) {
                    size_t obj_start = arr.find('{', pos);

                    if (obj_start == string::npos) {
                        break;
                    }

                    size_t obj_end = arr.find('}', obj_start);

                    if (obj_end == string::npos) {
                        break;
                    }

                    string obj = arr.substr(obj_start, obj_end - obj_start + 1);
                    string gname = get_field(obj, "name");
                    vector<string> members = extract_list(obj, "members");

                    if (!gname.empty() && !members.empty()) {
                        clubs[gname] = { gname, members };
                    }

                    pos = obj_end + 1;
                }
            }
        }

        cout << "[OK] Loaded " << folks.size() << " users, " << total_links << " friendships, " << clubs.size() << " groups." << endl;
        return true;
    }







    bool save_to_file(const string& filename) const {
        ofstream file(filename);

        if (!file.is_open()) {
            cerr << "[Error] Cannot create file!" << endl;
            return false;
        }

        file << "{\n  \"users\": [\n";
        bool first = true;

        for (unordered_map<string, Person>::const_iterator it = folks.begin(); it != folks.end(); ++it) {
            const string& id = it->first;
            const Person& p = it->second;

            if (!first) {
                file << ",\n";
            }

            file << "    {\"id\": \"" << escape_chars(id) << "\", \"name\": \"" << escape_chars(p.full_name) << "\"}";
            first = false;
        }

        file << "\n  ],\n  \"edges\": [\n";

        first = true;

        for (unordered_map<string, vector<string>>::const_iterator it = adj.begin(); it != adj.end(); ++it) {
            
            const string& u = it->first;
            
            const vector<string>& neighbors = it->second;
            for (const string& v : neighbors) {
                if (u < v) {
                    if (!first) file << ",\n";
                    file << "    {\"source\": \"" << escape_chars(u) << "\", \"target\": \"" << escape_chars(v) << "\", \"weight\": 1}";
                    first = false;
                }
            }
        }

        file << "\n  ],\n  \"groups\": [\n";

        first = true;

        for (unordered_map<string, Circle>::const_iterator it = clubs.begin(); it != clubs.end(); ++it) {
            
            const string& name = it->first;
            const Circle& c = it->second;
            
            if (!first) {
                file << ",\n";
            }
            
            file << "    {\"name\": \"" << escape_chars(name) << "\", \"members\": [";
            bool firstMember = true;

            for (const string& m : c.participants) {
                
                if (!firstMember) {
                    file << ", ";
                }

                file << "\"" << escape_chars(m) << "\"";
                firstMember = false;
            }

            file << "]}";
            first = false;
        }

        file << "\n  ]\n}\n";
        file.close();
        cout << "[OK] Saved to " << filename << endl;

        return true;
    }







    void clear_all() {
        folks.clear();
        adj.clear();
        adj_set.clear();
        clubs.clear();
        total_links = 0;
    }






    bool add_person(
        const string& id, 
        const string& name
    ) {
        if (is_existing(id)) {
            cout << "[Warning] User " << id << " already exists." << endl;
            return false;
        }

        folks[id] = { id, name };
        adj[id] = vector<string>();
        adj_set[id] = unordered_set<string>();
        cout << "[OK] User " << id << " (" << name << ") added." << endl;

        return true;
    }






    bool remove_person(const string& id) {
        if (!is_existing(id)) {
            cout << "[Error] User " << id << " not found!" << endl;
            return false;
        }

        for (unordered_map<string, vector<string>>::iterator it = adj.begin(); it != adj.end(); ++it) {
            const string& uid = it->first;
            vector<string>& neighbors = it->second;
            
            if (uid == id) {
                continue;
            }

            vector<string>::iterator fit = find(neighbors.begin(), neighbors.end(), id);
            
            if (fit != neighbors.end()) {
                neighbors.erase(fit);
                adj_set[uid].erase(id);
                total_links--;
            }
        }

        total_links -= (int)adj[id].size();
        adj.erase(id);
        adj_set.erase(id);
        folks.erase(id);
        
        vector<string> to_remove;

        for (unordered_map<string, Circle>::iterator it = clubs.begin(); it != clubs.end(); ++it) {
            const string& gname = it->first;
            Circle& c = it->second;
            
            vector<string>::iterator git = find(c.participants.begin(), c.participants.end(), id);
            if (git != c.participants.end()) {
                c.participants.erase(git);
                if (c.participants.size() < 2) to_remove.push_back(gname);
            }
        }

        for (const string& gname : to_remove) {
            clubs.erase(gname);
        }

        cout << "[OK] User " << id << " removed." << endl;
        return true;
    }







    bool rename_person(
        const string& id, 
        const string& new_name
    ) {
        if (!is_existing(id)) {
            cout << "[Error] User " << id << " not found!" << endl;
            return false;
        }

        folks[id].full_name = new_name;
        cout << "[OK] User " << id << " renamed to " << new_name << "." << endl;

        return true;
    }







    bool add_link(
        const string& u, 
        const string& v, 
        int weight = 1
    ) {
        
        if (!is_existing(u) || !is_existing(v)) {
            cout << "[Error] One or both users do not exist!" << endl;
            return false;
        }

        if (u == v) {
            cout << "[Error] A user cannot be friends with themselves!" << endl;
            return false;
        }

        if (is_linked(u, v)) {
            cout << "[Warning] Friendship already exists." << endl;
            return false;
        }

        add_link_internal(u, v);
        cout << "[OK] Friendship between " << u << " and " << v << " created." << endl;

        return true;
    }







    void add_link_internal(
        const string& u, 
        const string& v
    ) {
        adj[u].push_back(v);
        adj[v].push_back(u);
        adj_set[u].insert(v);
        adj_set[v].insert(u);

        total_links++;
    }







    bool remove_link(
        const string& u, 
        const string& v
    ) {
        if (!is_existing(u) || !is_existing(v)) {
            cout << "[Error] One or both users not found!" << endl;
            return false;
        }

        if (!is_linked(u, v)) {
            cout << "[Error] Friendship does not exist!" << endl;
            return false;
        }

        vector<string>& uList = adj[u];
        uList.erase(remove(uList.begin(), uList.end(), v), uList.end());

        vector<string>& vList = adj[v];
        vList.erase(remove(vList.begin(), vList.end(), u), vList.end());

        adj_set[u].erase(v);
        adj_set[v].erase(u);

        total_links--;

        cout << "[OK] Friendship between " << u << " and " << v << " removed." << endl;

        return true;
    }







    void show_friends(const string& id) const {

        if (!is_existing(id)) {
            cout << "[Error] User " << id << " not found!" << endl;
            return;
        }

        unordered_map<string, vector<string>>::const_iterator it = adj.find(id);
        if (it == adj.end() || it->second.empty()) {
            cout << "User " << id << " has no friends." << endl;
            return;
        }

        cout << "Friends of " << id << " (" << folks.at(id).full_name << "): " << it->second.size() << endl;
        cout << "----------------------------------------" << endl;

        for (const string& fid : it->second) {
            unordered_map<string, Person>::const_iterator pit = folks.find(fid);
            cout << "  - " << fid;
            if (pit != folks.end()) cout << " (" << pit->second.full_name << ")";
            cout << endl;
        }
    }







    void check_connectivity(
        const string& u, 
        const string& v
    ) const {
        if (!is_existing(u) || !is_existing(v)) {
            cout << "[Error] One or both users not found!" << endl;
            return;
        }

        if (u == v) {
            cout << "Yes, a user is connected to themselves." << endl;
            return;
        }

        unordered_map<string, int> dist;
        unordered_map<string, string> parent;
        run_bfs(u, dist, parent);

        if (dist.find(v) != dist.end()) {
            cout << "Yes, " << u << " and " << v << " are connected." << endl;
            cout << "Distance: " << dist.at(v) << " edges" << endl;
        }
        else {
            cout << "No, " << u << " and " << v << " are in separate groups." << endl;
        }
    }







    void findPath(
        const string& u, 
        const string& v
    ) const {

        if (!is_existing(u) || !is_existing(v)) {
            cout << "[Error] One or both users not found!" << endl;
            return;
        }

        if (u == v) {
            cout << "Source and destination are the same: " << u << endl;
            return;
        }

        unordered_map<string, int> dist;
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
            unordered_map<string, string>::const_iterator it = parent.find(cur);
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

        if (!is_existing(id)) {
            cout << "[Error] User " << id << " not found!" << endl;
            return;
        }

        unordered_map<string, unordered_set<string>>::const_iterator it = adj_set.find(id);
        if (it == adj_set.end() || it->second.empty()) {
            cout << "User " << id << " has no friends for suggestions." << endl;
            return;
        }

        unordered_map<string, int> mutualCount;
        for (const string& fid : it->second) {

            unordered_map<string, vector<string>>::const_iterator fit = adj.find(fid);
            if (fit == adj.end()) {
                continue;
            }

            for (const string& cand : fit->second) {
                if (cand == id) {
                    continue;
                }

                if (adj_set.at(id).find(cand) != adj_set.at(id).end()) {
                    continue;
                }

                mutualCount[cand]++;
            }
        }

        if (mutualCount.empty()) {
            cout << "No suggestions for " << id << "." << endl;
            return;
        }

        vector<pair<string, int> > suggestions(mutualCount.begin(), mutualCount.end());

        sort(suggestions.begin(), suggestions.end(), [](
            const pair<string, int>& a, 
            const pair<string, int>& b
        ) {
            return a.second > b.second;
        });

        cout << "Friend suggestions for " << id << " (" << folks.at(id).full_name << "):" << endl;
        cout << "----------------------------------------" << endl;

        for (vector<pair<string, int> >::const_iterator sit = suggestions.begin(); sit != suggestions.end(); ++sit) {
            const string& cand = sit->first;
            int count = sit->second;

            unordered_map<string, Person>::const_iterator pit = folks.find(cand);

            cout << "  - " << cand;

            if (pit != folks.end()) {
                cout << " (" << pit->second.full_name << ")";
            }

            cout << " | Mutual friends: " << count << endl;
        }
    }







    void display_groups() const {

        if (folks.empty()) {
            cout << "Network is empty." << endl;
            return;
        }

        vector<vector<string> > cliques;
        unordered_set<string> current, pending, processed;

        for (unordered_map<string, Person>::const_iterator it = folks.begin(); it != folks.end(); ++it) {
            pending.insert(it->first);
        }

        find_cliques(current, pending, processed, cliques);

        sort(cliques.begin(), cliques.end(), [](
            const vector<string>& a, 
            const vector<string>& b
        ) {
            return a.size() > b.size();
        });

        vector<vector<string> > unique_cliques;

        for (vector<vector<string> >::iterator cit = cliques.begin(); cit != cliques.end(); ++cit) {
            vector<string>& c = *cit;
            sort(c.begin(), c.end());

            bool dup = false;

            for (vector<vector<string> >::const_iterator ucit = unique_cliques.begin(); ucit != unique_cliques.end(); ++ucit) {
                if (*ucit == c) { 
                    dup = true; 
                    break;
                }
            }

            if (!dup) {
                unique_cliques.push_back(c);
            }
        }

        cout << "Number of maximal cliques (groups): " << unique_cliques.size() << endl;
        cout << "===================================" << endl;

        for (size_t i = 0; i < unique_cliques.size(); i++) {
            cout << "Group " << (i + 1) << " (" << unique_cliques[i].size() << " members):" << endl;

            for (const string& member : unique_cliques[i]) {
                unordered_map<string, Person>::const_iterator pit = folks.find(member);
                cout << "  - " << member;

                if (pit != folks.end()) {
                    cout << " (" << pit->second.full_name << ")";
                }

                cout << endl;
            }

            if (i < unique_cliques.size() - 1) {
                cout << "----------------------------------------" << endl;
            }
        }
    }








    bool add_circle(
        const string& name, 
        const vector<string>& members
    ) {

        if (clubs.find(name) != clubs.end()) {
            cout << "[Error] Group " << name << " already exists!" << endl;
            return false;
        }

        if (members.size() < 2) {
            cout << "[Error] A group must have at least 2 members." << endl;
            return false;
        }

        for (const string& m : members) {
            if (!is_existing(m)) {
                cout << "[Error] User " << m << " does not exist!" << endl;
                return false;
            }
        }

        bool added_any = false;

        for (size_t i = 0; i < members.size(); i++) {
            for (size_t j = i + 1; j < members.size(); j++) {

                if (!is_linked(members[i], members[j])) {
                    add_link_internal(members[i], members[j]);
                    added_any = true;
                }
            }
        }

        clubs[name] = { name, members };

        cout << "[OK] Group " << name << " created with " << members.size() << " members.";
        if (added_any) {
            cout << " New friendships were created.";
        }

        cout << endl;

        return true;
    }









    bool remove_circle(const string& name) {
        unordered_map<string, Circle>::iterator it = clubs.find(name);
        if (it == clubs.end()) {
            cout << "[Error] Group " << name << " not found!" << endl;
            return false;
        }

        const vector<string>& members = it->second.participants;
        for (size_t i = 0; i < members.size(); i++) {
            for (size_t j = i + 1; j < members.size(); j++) {

                if (is_linked(members[i], members[j])) {
                    vector<string>& uList = adj[members[i]];
                    uList.erase(remove(uList.begin(), uList.end(), members[j]), uList.end());
                    vector<string>& vList = adj[members[j]];
                    vList.erase(remove(vList.begin(), vList.end(), members[i]), vList.end());
                    adj_set[members[i]].erase(members[j]);
                    adj_set[members[j]].erase(members[i]);
                    total_links--;
                }
            }
        }

        clubs.erase(it);
        cout << "[OK] Group " << name << " removed. All internal friendships deleted." << endl;

        return true;
    }







    void show_circles() const {
        if (clubs.empty()) {
            cout << "No defined groups." << endl;
            return;
        }

        cout << "Defined Groups (" << clubs.size() << "):" << endl;
        cout << "===================================" << endl;

        for (unordered_map<string, Circle>::const_iterator it = clubs.begin(); it != clubs.end(); ++it) {
            const string& name = it->first;
            const Circle& c = it->second;
            cout << "Group: " << name << " (" << c.participants.size() << " members)" << endl;

            for (const string& m : c.participants) {
                unordered_map<string, Person>::const_iterator pit = folks.find(m);
                cout << "  - " << m;
                if (pit != folks.end()) cout << " (" << pit->second.full_name << ")";
                cout << endl;
            }

            cout << "----------------------------------------" << endl;
        }
    }








    void show_popular() const {

        if (folks.empty()) {
            cout << "Network is empty." << endl;
            return;
        }

        int max_friends = -1;

        for (unordered_map<string, vector<string>>::const_iterator it = adj.begin(); it != adj.end(); ++it) {
            const vector<string>& neighbors = it->second;
            max_friends = max(max_friends, (int)neighbors.size());
        }

        if (max_friends <= 0) {
            cout << "No friendships in the network." << endl;
            return;
        }

        cout << "Users with most friends (" << max_friends << " friends):" << endl;
        cout << "----------------------------------------" << endl;

        for (unordered_map<string, vector<string>>::const_iterator it = adj.begin(); it != adj.end(); ++it) {
            const string& id = it->first;
            const vector<string>& neighbors = it->second;

            if ((int)neighbors.size() == max_friends) {
                unordered_map<string, Person>::const_iterator pit = folks.find(id);
                cout << "  - " << id;
                if (pit != folks.end()) cout << " (" << pit->second.full_name << ")";
                cout << endl;
            }
        }
    }







    void show_mutual(
        const string& u, 
        const string& v
    ) const {

        if (!is_existing(u) || !is_existing(v)) {
            cout << "[Error] One or both users not found!" << endl;
            return;
        }

        if (u == v) {
            cout << "Mutual friends of a user with themselves are all their friends." << endl;
            show_friends(u);
            return;
        }

        unordered_map<string, unordered_set<string>>::const_iterator uit = adj_set.find(u);
        unordered_map<string, unordered_set<string>>::const_iterator vit = adj_set.find(v);

        if (uit == adj_set.end() || vit == adj_set.end()) {
            cout << "Mutual friends: 0" << endl;
            return;
        }

        vector<string> common;
        const unordered_set<string>& u_friends = uit->second;
        const unordered_set<string>& v_friends = vit->second;

        if (u_friends.size() > v_friends.size()) {
            for (unordered_set<string>::const_iterator fit = v_friends.begin(); fit != v_friends.end(); ++fit) {
                const string& f = *fit;
                if (u_friends.find(f) != u_friends.end()) {
                    common.push_back(f);
                }
            }
        }
        else {
            for (unordered_set<string>::const_iterator fit = u_friends.begin(); fit != u_friends.end(); ++fit) {
                const string& f = *fit;
                if (v_friends.find(f) != v_friends.end()) {
                    common.push_back(f);
                }
            }
        }

        cout << "Mutual friends of " << u << " and " << v << ": " << common.size() << endl;
        cout << "----------------------------------------" << endl;

        for (const string& f : common) {
            unordered_map<string, Person>::const_iterator pit = folks.find(f);
            cout << "  - " << f;

            if (pit != folks.end()) {
                cout << " (" << pit->second.full_name << ")";
            }

            cout << endl;
        }
    }







    void show_stats() const {
        cout << "========== Network Statistics ==========" << endl;
        cout << "a. Total users: " << folks.size() << endl;
        cout << "b. Total friendships: " << total_links << endl;

        if (folks.empty()) {
            cout << "c. Average degree: 0" << endl;
            cout << "d. Largest clique: 0" << endl;
            cout << "e. Most connected user: -" << endl;

            return;
        }

        double avg_degree = (2.0 * total_links) / folks.size();
        cout << "c. Average degree: " << fixed << setprecision(2) << avg_degree << endl;

        vector<vector<string> > cliques;
        unordered_set<string> current, pending, processed;

        for (unordered_map<string, Person>::const_iterator it = folks.begin(); it != folks.end(); ++it) {
            pending.insert(it->first);
        }

        find_cliques(current, pending, processed, cliques);
        int largest_clique = 0;

        for (vector<vector<string> >::const_iterator cit = cliques.begin(); cit != cliques.end(); ++cit) {
            largest_clique = max(largest_clique, (int)cit->size());
        }

        string max_connected;
        int max_degree = -1;
        for (unordered_map<string, vector<string>>::const_iterator it = adj.begin(); it != adj.end(); ++it) {
            
            const string& id = it->first;
            const vector<string>& neighbors = it->second;
            int deg = (int)neighbors.size();

            if (deg > max_degree) {
                max_degree = deg;
                max_connected = id;
            }
        }

        cout << "d. Largest clique size: " << largest_clique << endl;
        unordered_map<string, Person>::const_iterator pit = folks.find(max_connected);
        cout << "e. Most connected user: " << max_connected;
        if (pit != folks.end()) {
            cout << " (" << pit->second.full_name << ") - " << max_degree << " friends";
        }

        cout << endl;
    }







    void show_distances(const string& id) const {
        if (!is_existing(id)) {
            cout << "[Error] User " << id << " not found!" << endl;
            return;
        }

        unordered_map<string, int> dist;
        unordered_map<string, string> parent;
        run_bfs(id, dist, parent);

        vector<pair<string, int> > results;
        for (unordered_map<string, Person>::const_iterator it = folks.begin(); it != folks.end(); ++it) {
            const string& uid = it->first;
            unordered_map<string, int>::const_iterator dit = dist.find(uid);

            if (dit != dist.end()) {
                results.push_back(make_pair(uid, dit->second));
            }
            else {
                results.push_back(make_pair(uid, -1));
            }
        }

        sort(results.begin(), results.end(), [](
            const pair<string,int>& a, 
            const pair<string,int>& b
        ) {
            if (a.second == -1 && b.second == -1) {
                return a.first < b.first;
            }

            if (a.second == -1) {
                return false;
            }

            if (b.second == -1) {
                return true;
            }

            return a.second < b.second;
        });

        cout << "Distances from " << id << " (" << folks.at(id).full_name << "):" << endl;
        cout << "----------------------------------------" << endl;

        for (vector<pair<string, int> >::const_iterator rit = results.begin(); rit != results.end(); ++rit) {
            const string& uid = rit->first;
            int d = rit->second;
            unordered_map<string, Person>::const_iterator pit = folks.find(uid);
            cout << "  " << uid;
            if (pit != folks.end()) {
                cout << " (" << pit->second.full_name << ")";
            }

            cout << " : ";

            if (d == -1) {
                cout << "INF (unreachable)";
            }
            else if (d == 0) {
                cout << "0 (self)";
            }
            else {
                cout << d << " edges";
            }

            cout << endl;
        }
    }







    void show_all() const {
        if (folks.empty()) {
            cout << "No users in the network." << endl;
            return;
        }

        cout << "User list (" << folks.size() << " users):" << endl;
        cout << "----------------------------------------" << endl;

        for (unordered_map<string, Person>::const_iterator it = folks.begin(); it != folks.end(); ++it) {
            const string& id = it->first;
            const Person& p = it->second;
            unordered_map<string, vector<string>>::const_iterator ait = adj.find(id);
            int friends = (ait != adj.end()) ? (int)ait->second.size() : 0;
            cout << "  " << id << " (" << p.full_name << ") - " << friends << " friends" << endl;
        }
    }
};







void show_header() {
    cout << "===================================" << endl;
    cout << "        Social Network Analyzer         " << endl;
    cout << "========================================" << endl;
}





void show_commands() {

    cout << "\n========== Command Reference ==========\n" << endl;

    cout << "[Data Management]" << endl;
    cout << "  load [filename]          Load network from JSON file" << endl;
    cout << "  save [filename]          Save network to JSON file" << endl;
    cout << "  clear                    Clear entire network" << endl;

    cout << "\n[User Management]" << endl;
    cout << "  users                    List all users" << endl;
    cout << "  adduser <id> <name>      Add a new user" << endl;
    cout << "  removeuser <id>         Remove a user" << endl;
    cout << "  rename <id> <new_name>  Rename a user" << endl;

    cout << "\n[Friendship Management]" << endl;
    cout << "  addfriend <id1> <id2>    Create friendship" << endl;
    cout << "  removefriend <id1> <id2> Remove friendship" << endl;

    cout << "\n[Search & Analysis]" << endl;
    cout << "  friends <id>             List friends of a user" << endl;
    cout << "  connected <id1> <id2>    Check if two users are connected" << endl;
    cout << "  path <id1> <id2>         Shortest path (BFS)" << endl;
    cout << "  suggest <id>             Suggest friends (mutual count)" << endl;
    cout << "  mutual <id1> <id2>       Mutual friends" << endl;
    cout << "  distances <id>           Distance to all users (sorted)" << endl;

    cout << "\n[Groups (Maximal Cliques)]" << endl;
    cout << "  groups                   List all maximal cliques in graph" << endl;
    cout << "  addgroup <name> <m1> <m2> [...]  Create a clique group" << endl;
    cout << "  removegroup <name>        Remove a group (delete internal edges)" << endl;
    cout << "  listgroups                List user-defined groups" << endl;

    cout << "\n[Statistics]" << endl;
    cout << "  popular                  Users with most friends" << endl;
    cout << "  stats                    Full network statistics" << endl;

    cout << "\n[Other]" << endl;
    cout << "  help                     Show this help" << endl;
    cout << "  exit                     Exit program" << endl;

    cout << "\n========================================" << endl;
    cout << "Tip: Run 'load data.json' on first use." << endl;
    cout << "========================================\n" << endl;
}






vector<string> split_input(const string& cmd) {
    vector<string> parts;
    stringstream ss(cmd);
    string part;

    while (ss >> part) {
        parts.push_back(part);
    }

    return parts;
}








int main() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    FriendNet network;
    string input;

    show_header();
    show_commands();

    while (true) {
        cout << "\n>> ";
        getline(cin, input);

        size_t start = input.find_first_not_of(" \t\r\n");
        if (start == string::npos) continue;
        input = input.substr(start);

        vector<string> args = split_input(input);
        if (args.empty()) continue;

        string cmd = args[0];
        transform(cmd.begin(), cmd.end(), cmd.begin(), ::tolower);

        try {
            if (cmd == "exit" || cmd == "quit") {
                cout << "Goodbye!" << endl;
                break;
            }
            else if (cmd == "help") {
                show_commands();
            }
            else if (cmd == "load") {
                string filename = (args.size() > 1) ? args[1] : "data.json";
                network.load_from_file(filename);
            }
            else if (cmd == "save") {
                string filename = (args.size() > 1) ? args[1] : "network.json";
                network.save_to_file(filename);
            }
            else if (cmd == "clear") {
                network.clear_all();
                cout << "[OK] Network cleared." << endl;
            }
            else if (cmd == "users") {
                network.show_all();
            }
            else if (cmd == "adduser") {
                if (args.size() < 3) {
                    cout << "[Error] Usage: adduser <id> <name>" << endl;
                }
                else {
                    string name = args[2];
                    for (size_t i = 3; i < args.size(); i++) name += " " + args[i];
                    network.add_person(args[1], name);
                }
            }
            else if (cmd == "removeuser") {
                if (args.size() < 2) cout << "[Error] Usage: removeuser <id>" << endl;
                else network.remove_person(args[1]);
            }
            else if (cmd == "rename") {
                if (args.size() < 3) {
                    cout << "[Error] Usage: rename <id> <new_name>" << endl;
                }
                else {
                    string name = args[2];
                    for (size_t i = 3; i < args.size(); i++) name += " " + args[i];
                    network.rename_person(args[1], name);
                }
            }
            else if (cmd == "addfriend") {
                if (args.size() < 3) cout << "[Error] Usage: addfriend <id1> <id2>" << endl;
                else network.add_link(args[1], args[2]);
            }
            else if (cmd == "removefriend") {
                if (args.size() < 3) cout << "[Error] Usage: removefriend <id1> <id2>" << endl;
                else network.remove_link(args[1], args[2]);
            }
            else if (cmd == "friends") {
                if (args.size() < 2) cout << "[Error] Usage: friends <id>" << endl;
                else network.show_friends(args[1]);
            }
            else if (cmd == "connected") {
                if (args.size() < 3) cout << "[Error] Usage: connected <id1> <id2>" << endl;
                else network.check_connectivity(args[1], args[2]);
            }
            else if (cmd == "path") {
                if (args.size() < 3) cout << "[Error] Usage: path <id1> <id2>" << endl;
                else network.findPath(args[1], args[2]);
            }
            else if (cmd == "suggest") {
                if (args.size() < 2) cout << "[Error] Usage: suggest <id>" << endl;
                else network.suggest_friends(args[1]);
            }
            else if (cmd == "groups") {
                network.display_groups();
            }
            else if (cmd == "addgroup") {
                if (args.size() < 4) {
                    cout << "[Error] Usage: addgroup <name> <member1> <member2> [...]" << endl;
                }
                else {
                    vector<string> members;
                    for (size_t i = 2; i < args.size(); i++) members.push_back(args[i]);
                    network.add_circle(args[1], members);
                }
            }
            else if (cmd == "removegroup") {
                if (args.size() < 2) cout << "[Error] Usage: removegroup <name>" << endl;
                else network.remove_circle(args[1]);
            }
            else if (cmd == "listgroups") {
                network.show_circles();
            }
            else if (cmd == "popular") {
                network.show_popular();
            }
            else if (cmd == "mutual") {
                if (args.size() < 3) cout << "[Error] Usage: mutual <id1> <id2>" << endl;
                else network.show_mutual(args[1], args[2]);
            }
            else if (cmd == "stats") {
                network.show_stats();
            }
            else if (cmd == "distances") {
                if (args.size() < 2) cout << "[Error] Usage: distances <id>" << endl;
                else network.show_distances(args[1]);
            }
            else {
                cout << "[Error] Unknown command! Type 'help' for assistance." << endl;
            }
        }
        catch (const exception& e) {
            cerr << "[System Error] " << e.what() << endl;
        }
    }

    return 0;
}