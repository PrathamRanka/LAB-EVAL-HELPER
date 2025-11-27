#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <algorithm>
#include <cctype>
#include <map>
#include <set>
#include <cmath>

using namespace std;

// Struct to hold each question
struct Question {
    string id;
    string question;
    string answer;
    vector<string> keywords;  // Extracted keywords for faster matching
    map<string, int> wordFreq; // Word frequency for TF-IDF
};

// Function to trim whitespace
string trim(const string& str) {
    size_t first = str.find_first_not_of(" \n\r\t\"");
    if (first == string::npos) return "";
    size_t last = str.find_last_not_of(" \n\r\t\"");
    return str.substr(first, (last - first + 1));
}

// Convert string to lowercase for case-insensitive matching
string toLower(const string& str) {
    string result = str;
    transform(result.begin(), result.end(), result.begin(), ::tolower);
    return result;
}

// Offline synonym/related terms mapping (DSA domain specific)
map<string, vector<string>> buildSynonymMap() {
    map<string, vector<string>> synonyms;
    synonyms["queue"] = {"q", "queues", "enqueue", "dequeue", "fifo"};
    synonyms["stack"] = {"stk", "stacks", "push", "pop", "lifo"};
    synonyms["tree"] = {"trees", "binary", "bst", "binarytree", "node"};
    synonyms["sort"] = {"sorting", "arrange", "order", "sorted"};
    synonyms["search"] = {"searching", "find", "lookup", "locate"};
    synonyms["linked"] = {"list", "linkedlist", "link", "node"};
    synonyms["graph"] = {"graphs", "bfs", "dfs", "dijkstra", "prim", "kruskal"};
    synonyms["hash"] = {"hashing", "hashtable", "hashmap", "map"};
    synonyms["circular"] = {"circle", "cyclic", "round", "circ"};
    synonyms["array"] = {"arr", "arrays", "list"};
    synonyms["string"] = {"str", "strings", "text", "char"};
    synonyms["merge"] = {"mergesort", "merging", "combine"};
    synonyms["quick"] = {"quicksort", "qsort"};
    synonyms["bubble"] = {"bubblesort"};
    synonyms["selection"] = {"selectionsort"};
    synonyms["insertion"] = {"insertionsort"};
    return synonyms;
}

// Levenshtein distance for typo tolerance
int levenshteinDistance(const string& s1, const string& s2) {
    int m = s1.length(), n = s2.length();
    vector<vector<int>> dp(m + 1, vector<int>(n + 1));
    
    for (int i = 0; i <= m; i++) dp[i][0] = i;
    for (int j = 0; j <= n; j++) dp[0][j] = j;
    
    for (int i = 1; i <= m; i++) {
        for (int j = 1; j <= n; j++) {
            if (s1[i-1] == s2[j-1]) {
                dp[i][j] = dp[i-1][j-1];
            } else {
                dp[i][j] = 1 + min({dp[i-1][j], dp[i][j-1], dp[i-1][j-1]});
            }
        }
    }
    return dp[m][n];
}

// Generate n-grams for partial matching
vector<string> getNGrams(const string& word, int n = 3) {
    vector<string> ngrams;
    if (word.length() < n) {
        ngrams.push_back(word);
        return ngrams;
    }
    for (size_t i = 0; i <= word.length() - n; i++) {
        ngrams.push_back(word.substr(i, n));
    }
    return ngrams;
}

// Calculate n-gram similarity
float ngramSimilarity(const string& w1, const string& w2) {
    auto ng1 = getNGrams(w1, 3);
    auto ng2 = getNGrams(w2, 3);
    
    set<string> s1(ng1.begin(), ng1.end());
    set<string> s2(ng2.begin(), ng2.end());
    
    int common = 0;
    for (const auto& ng : s1) {
        if (s2.count(ng)) common++;
    }
    
    return (s1.empty() || s2.empty()) ? 0 : (2.0 * common) / (s1.size() + s2.size());
}

// Extract words from a string (with stop words removal)
vector<string> getWords(const string& str) {
    set<string> stopWords = {"the", "a", "an", "is", "are", "was", "were", "to", "of", "in", "on", "at", "for", "with", "from", "by"};
    istringstream stream(toLower(str));
    vector<string> words;
    string w;
    while (stream >> w) {
        // Remove punctuation
        w.erase(remove_if(w.begin(), w.end(), [](char c) { 
            return !isalnum(c); 
        }), w.end());
        if (!w.empty() && stopWords.find(w) == stopWords.end()) {
            words.push_back(w);
        }
    }
    return words;
}

// Expand query with synonyms
vector<string> expandWithSynonyms(const vector<string>& words, const map<string, vector<string>>& synonymMap) {
    set<string> expanded(words.begin(), words.end());
    
    for (const auto& word : words) {
        // Check if word is a key in synonym map
        if (synonymMap.count(word)) {
            for (const auto& syn : synonymMap.at(word)) {
                expanded.insert(syn);
            }
        }
        // Check if word is in any synonym list
        for (const auto& entry : synonymMap) {
            for (const auto& syn : entry.second) {
                if (syn == word) {
                    expanded.insert(entry.first);
                    for (const auto& s : entry.second) {
                        expanded.insert(s);
                    }
                    break;
                }
            }
        }
    }
    
    return vector<string>(expanded.begin(), expanded.end());
}

// Advanced ML-inspired similarity score (offline, no training needed)
float advancedSimilarity(const string& userInput, const Question& q, const map<string, vector<string>>& synonymMap) {
    vector<string> userWords = getWords(userInput);
    vector<string> expandedUserWords = expandWithSynonyms(userWords, synonymMap);
    vector<string> questionWords = getWords(q.question);
    
    if (userWords.empty()) return 0;
    
    float score = 0;
    
    // 1. Exact word matches (highest weight)
    for (const auto& uw : expandedUserWords) {
        for (const auto& qw : questionWords) {
            if (uw == qw) {
                score += 100;
            }
        }
    }
    
    // 2. Substring matches
    for (const auto& uw : userWords) {
        for (const auto& qw : questionWords) {
            if (uw.length() > 2 && qw.find(uw) != string::npos) {
                score += 50;
            }
            if (qw.length() > 2 && uw.find(qw) != string::npos) {
                score += 50;
            }
        }
    }
    
    // 3. Fuzzy matching with Levenshtein distance (typo tolerance)
    for (const auto& uw : userWords) {
        if (uw.length() < 3) continue;
        for (const auto& qw : questionWords) {
            if (qw.length() < 3) continue;
            int dist = levenshteinDistance(uw, qw);
            int maxLen = max(uw.length(), qw.length());
            if (dist <= 2 && maxLen > 3) {
                score += (1.0 - (float)dist / maxLen) * 40;
            }
        }
    }
    
    // 4. N-gram similarity for very fuzzy matches
    for (const auto& uw : userWords) {
        if (uw.length() < 4) continue;
        for (const auto& qw : questionWords) {
            if (qw.length() < 4) continue;
            float ngramSim = ngramSimilarity(uw, qw);
            if (ngramSim > 0.5) {
                score += ngramSim * 30;
            }
        }
    }
    
    // 5. Keyword density bonus
    if (!q.keywords.empty()) {
        for (const auto& uw : expandedUserWords) {
            for (const auto& kw : q.keywords) {
                if (uw == kw) score += 80;
            }
        }
    }
    
    // 6. TF-IDF style weighting (rare words matter more)
    for (const auto& uw : userWords) {
        if (q.wordFreq.count(uw)) {
            score += 60.0 / (1 + log(q.wordFreq.at(uw)));
        }
    }
    
    // 7. Query coverage bonus (how much of user input is matched)
    float coverage = 0;
    for (const auto& uw : userWords) {
        for (const auto& qw : questionWords) {
            if (uw == qw || qw.find(uw) != string::npos) {
                coverage += 1.0;
                break;
            }
        }
    }
    score += (coverage / userWords.size()) * 50;
    
    return score;
}

// Parse JSON file and extract questions
vector<Question> loadQuestions(const string& filename) {
    vector<Question> questions;
    ifstream file(filename);
    if (!file.is_open()) {
        cout << "Failed to open file: " << filename << endl;
        return questions;
    }

    stringstream buffer;
    buffer << file.rdbuf();
    string content = buffer.str();
    file.close();

    // Simple JSON parser for the specific structure
    size_t pos = 0;
    while ((pos = content.find("\"id\":", pos)) != string::npos) {
        size_t idStart = content.find("\"", pos + 5) + 1;
        size_t idEnd = content.find("\"", idStart);
        string id = trim(content.substr(idStart, idEnd - idStart));

        size_t qStart = content.find("\"question\":", idEnd);
        if (qStart == string::npos) break;
        qStart = content.find("\"", qStart + 11) + 1;
        size_t qEnd = content.find("\",", qStart);
        if (qEnd == string::npos) qEnd = content.find("\"", qStart);
        string question = trim(content.substr(qStart, qEnd - qStart));

        size_t aStart = content.find("\"answer\":", qEnd);
        if (aStart == string::npos) break;
        aStart = content.find("\"", aStart + 9) + 1;
        size_t aEnd = content.find("\"\n    }", aStart);
        if (aEnd == string::npos) aEnd = content.find("\"\n  }", aStart);
        if (aEnd == string::npos) aEnd = content.find("\"", aStart + 100);
        string answer = content.substr(aStart, aEnd - aStart);

        // Unescape newlines
        size_t escapePos = 0;
        while ((escapePos = answer.find("\\n", escapePos)) != string::npos) {
            answer.replace(escapePos, 2, "\n");
            escapePos++;
        }

        Question newQ;
        newQ.id = id;
        newQ.question = question;
        newQ.answer = answer;
        
        // Extract and index keywords
        vector<string> words = getWords(question);
        for (const auto& w : words) {
            if (w.length() > 3) {  // Only meaningful words
                newQ.keywords.push_back(w);
            }
            newQ.wordFreq[w]++;
        }
        
        questions.push_back(newQ);
        pos = aEnd;
    }

    return questions;
}

int main() {
    vector<Question> questions = loadQuestions("data.json");
    if (questions.empty()) {
        cout << "No questions loaded. Check your data file.\n";
        return 0;
    }

    // Build synonym map once (offline knowledge base)
    map<string, vector<string>> synonymMap = buildSynonymMap();

    cout << "=== DSA Lab Helper (Offline AI Mode) ===\n";
    cout << "Loaded " << questions.size() << " questions with smart matching!\n";
    cout << "\nEnter keywords (handles typos, synonyms, short phrases): ";
    string userInput;
    getline(cin, userInput);

    if (userInput.empty()) {
        cout << "No input provided.\n";
        return 0;
    }

    // Find top 3 matches instead of just 1
    vector<pair<float, int>> scores;
    
    for (size_t i = 0; i < questions.size(); i++) {
        float score = advancedSimilarity(userInput, questions[i], synonymMap);
        scores.push_back({score, i});
    }
    
    // Sort by score descending
    sort(scores.begin(), scores.end(), [](auto& a, auto& b) {
        return a.first > b.first;
    });
    
    float bestScore = scores[0].first;
    int bestIdx = scores[0].second;

    if (bestScore > 10) {  // Minimum threshold
        cout << "\n" << string(60, '=') << "\n";
        cout << "BEST MATCH (Confidence: " << (int)min(100.0f, bestScore/5) << "%)\n";
        cout << string(60, '=') << "\n";
        cout << "ID: " << questions[bestIdx].id << endl;
        cout << "Question: " << questions[bestIdx].question << endl;
        cout << "\n--- ANSWER ---\n";
        cout << questions[bestIdx].answer << endl;
        cout << string(60, '=') << "\n";
        
        // Show alternative matches if available
        if (scores.size() > 1 && scores[1].first > 30) {
            cout << "\n[Other possible matches:]\n";
            for (size_t i = 1; i < min((size_t)3, scores.size()); i++) {
                if (scores[i].first > 30) {
                    cout << "  - " << questions[scores[i].second].id 
                         << ": " << questions[scores[i].second].question.substr(0, 80) 
                         << "...\n";
                }
            }
        }
    } else {
        cout << "\nNo confident match found. Try different keywords.\n";
        cout << "Tip: Use terms like queue, stack, tree, sort, graph, bfs, linked, etc.\n";
        cout << "Typos are OK! (e.g., 'curcular que' works)\n";
    }

    return 0;
}
