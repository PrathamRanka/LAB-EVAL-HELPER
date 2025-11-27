#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <algorithm>
#include <cctype>

using namespace std;

// Struct to hold each question
struct Question {
    string id;
    string question;
    string answer;
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

// Extract words from a string
vector<string> getWords(const string& str) {
    istringstream stream(toLower(str));
    vector<string> words;
    string w;
    while (stream >> w) {
        // Remove punctuation
        w.erase(remove_if(w.begin(), w.end(), [](char c) { 
            return !isalnum(c); 
        }), w.end());
        if (!w.empty()) words.push_back(w);
    }
    return words;
}

// Advanced similarity score with partial matching
int similarity(const string& userInput, const string& questionText) {
    vector<string> userWords = getWords(userInput);
    vector<string> questionWords = getWords(questionText);
    
    if (userWords.empty()) return 0;
    
    int score = 0;
    
    // Exact word matches
    for (const auto& uw : userWords) {
        for (const auto& qw : questionWords) {
            if (uw == qw) {
                score += 10; // Higher weight for exact matches
            } else if (qw.find(uw) != string::npos || uw.find(qw) != string::npos) {
                score += 5; // Partial match (substring)
            }
        }
    }
    
    // Bonus for matching multiple words
    score += userWords.size() * 2;
    
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

        questions.push_back({id, question, answer});
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

    cout << "Loaded " << questions.size() << " questions successfully!\n";
    cout << "Enter your DSA question (or keywords): ";
    string userInput;
    getline(cin, userInput);

    if (userInput.empty()) {
        cout << "No input provided.\n";
        return 0;
    }

    Question* bestMatch = nullptr;
    int bestScore = -1;

    for (auto &q : questions) {
        int score = similarity(userInput, q.question);
        if (score > bestScore) {
            bestScore = score;
            bestMatch = &q;
        }
    }

    if (bestMatch && bestScore > 0) {
        cout << "\n========================================\n";
        cout << "Closest match found (Score: " << bestScore << "):\n";
        cout << "========================================\n";
        cout << "ID: " << bestMatch->id << endl;
        cout << "Question: " << bestMatch->question << endl;
        cout << "\n--- ANSWER ---\n";
        cout << bestMatch->answer << endl;
        cout << "========================================\n";
    } else {
        cout << "\nNo similar question found. Try different keywords.\n";
        cout << "Try terms like: queue, stack, tree, sort, search, graph, hash, etc.\n";
    }

    return 0;
}
