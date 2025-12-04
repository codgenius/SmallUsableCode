#include<string>

#include <iostream>
#include <string>
#include <unordered_map>
using namespace std;
struct trie_node {
    bool is_end_of_word;
    trie_node* children[26];
    trie_node() : is_end_of_word(false) {
        for (int i = 0; i < 26; ++i) {
            children[i] = nullptr;
        }
    }
};
class Trie {
private:
    trie_node* root;    
public:
    Trie() {
        root = new trie_node();
    }
    trie_node* getRoot() {
        return root;
    }
    void insert(const std::string& word) {
        trie_node* current = root;
        for (char ch : word){
            int index = ch-'a';
            if(current->children[index] == nullptr)
                current->children[index] = new trie_node();
            current = current->children[index];
        }
        current->is_end_of_word = true;
    }
    bool search(const std::string& word) {
        trie_node* current = root;
        for (char ch : word){
            int index = ch-'a';
            if(current->children[index] == nullptr)
                return current->is_end_of_word;
            current = current->children[index]; 
            if(current->is_end_of_word)
                return true;
        }
        return current->is_end_of_word;
    }
    // memo is to save each start index in order to know wither we run threw some words before
    bool wordBreak(const std::string& word, int start,std::unordered_map<int,bool> memo={}) {
        // string :pleaseApple, word dict :please apple ple
        // case of please apple return true 
        // case of ple ease apple return false
        // need to do recursion to break the word once ple is found check easeapple 
        // then check pleaseapple ... 
        // so we need to recursively call wordBreak on the suffixes
        // while traversing the trie if we find a word end we call wordBreak on the remaining suffix and  trie if we continue or we call the remaining suffix 
        //  
        if (start == word.length()) {
            return true;
        }
        if (memo.find(start) != memo.end()) {
            return memo[start];
        }

        trie_node* current = root;
        for (int i = start; i < word.length(); ++i) {
            int index = word[i] - 'a';
            if (current->children[index] == nullptr) {
                break;
            }
            current = current->children[index];
            if (current->is_end_of_word) {
                if (wordBreak(word, i + 1, memo)) {
                    memo[start] = true;
                    return true;
                }
            }
        }

        memo[start] = false;
        return false;
    }   
     bool canSegment(const std::string& s) {
        std::unordered_map<int, bool> memo;
        return wordBreak(s, 0, memo);
    }

};


// Assume your Trie class is already implemented above...

int main() {
    Trie trie;
    // Insert the dictionary words
    trie.insert("please");
    trie.insert("apple");
    trie.insert("ple");

    // Test case 1
    std::string s1 = "pleaseapple";
    std::cout << "Test 1: " << s1 << " → "
              << (trie.canSegment(s1) ? "True" : "False") << std::endl;

    // Test case 2 (should fail: ple + easeapple)
    std::string s2 = "pleeaseapple";
    std::cout << "Test 2: " << s2 << " → "
              << (trie.canSegment(s2) ? "True" : "False") << std::endl;

    // Test case 3 (edge: empty string)
    std::string s3 = "";
    std::cout << "Test 3: [empty] → "
              << (trie.canSegment(s3) ? "True" : "False") << std::endl;

    // Test case 4 (non-breakable)
    std::string s4 = "applepleapple";
    std::cout << "Test 4: " << s4 << " → "
              << (trie.canSegment(s4) ? "True" : "False") << std::endl;

    return 0;
}
