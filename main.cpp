#include <bits/stdc++.h>
using namespace std;

using i128 = __int128_t;

static const string ACCOUNTS_FILE = "bookstore_accounts.db";
static const string BOOKS_FILE = "bookstore_books.db";
static const string TRANSACTIONS_FILE = "bookstore_transactions.db";

struct Account {
    string password;
    string username;
    int privilege = 0;
};

struct Book {
    string isbn;
    string name;
    string author;
    vector<string> keywords;
    i128 price = 0;
    int stock = 0;
};

struct Transaction {
    i128 income = 0;
    i128 expenditure = 0;
    string type;
    string isbn;
    int quantity = 0;
};

struct LoginFrame {
    string userId;
    string selectedIsbn;
};

struct EmployeeStat {
    long long commands = 0;
};

struct TokenSpan {
    string text;
    size_t start = 0;
    size_t end = 0;
};

static inline string trim(const string &s) {
    size_t l = 0, r = s.size();
    while (l < r && s[l] == ' ') ++l;
    while (r > l && s[r - 1] == ' ') --r;
    return s.substr(l, r - l);
}

static vector<string> splitSpaces(const string &s) {
    vector<string> parts;
    size_t i = 0, n = s.size();
    while (i < n) {
        while (i < n && s[i] == ' ') ++i;
        if (i >= n) break;
        size_t j = i;
        while (j < n && s[j] != ' ') ++j;
        parts.push_back(s.substr(i, j - i));
        i = j;
    }
    return parts;
}

static vector<string> tokenizeCommand(const string &s) {
    vector<string> tokens;
    size_t i = 0, n = s.size();
    while (i < n) {
        while (i < n && s[i] == ' ') ++i;
        if (i >= n) break;
        if (s[i] == '"') {
            size_t j = i + 1;
            while (j < n && s[j] != '"') ++j;
            if (j >= n) {
                tokens.push_back(s.substr(i));
                break;
            }
            tokens.push_back(s.substr(i, j - i + 1));
            i = j + 1;
        } else {
            size_t j = i;
            while (j < n && s[j] != ' ') ++j;
            tokens.push_back(s.substr(i, j - i));
            i = j;
        }
    }
    return tokens;
}

static vector<TokenSpan> tokenizeWithSpans(const string &s) {
    vector<TokenSpan> tokens;
    size_t i = 0, n = s.size();
    while (i < n) {
        while (i < n && s[i] == ' ') ++i;
        if (i >= n) break;
        size_t start = i;
        if (s[i] == '"') {
            size_t j = i + 1;
            while (j < n && s[j] != '"') ++j;
            if (j >= n) {
                tokens.push_back({s.substr(i), start, n});
                break;
            }
            tokens.push_back({s.substr(i, j - i + 1), start, j + 1});
            i = j + 1;
        } else {
            size_t j = i;
            while (j < n && s[j] != ' ') ++j;
            tokens.push_back({s.substr(i, j - i), start, j});
            i = j;
        }
    }
    return tokens;
}

static vector<string> splitTab(const string &s) {
    vector<string> parts;
    string cur;
    for (char c : s) {
        if (c == '\t') {
            parts.push_back(cur);
            cur.clear();
        } else {
            cur.push_back(c);
        }
    }
    parts.push_back(cur);
    return parts;
}

static string joinKeywords(const vector<string> &keywords) {
    string result;
    for (size_t i = 0; i < keywords.size(); ++i) {
        if (i) result.push_back('|');
        result += keywords[i];
    }
    return result;
}

static vector<string> splitKeywords(const string &s) {
    vector<string> keywords;
    string cur;
    for (char c : s) {
        if (c == '|') {
            keywords.push_back(cur);
            cur.clear();
        } else {
            cur.push_back(c);
        }
    }
    if (!s.empty() || !cur.empty()) keywords.push_back(cur);
    return keywords;
}

static bool keywordListValid(const vector<string> &keywords) {
    if (keywords.empty()) return false;
    set<string> uniq;
    for (const auto &kw : keywords) {
        if (kw.empty()) return false;
        if (!uniq.insert(kw).second) return false;
    }
    return true;
}

static bool isDigits(const string &s) {
    return !s.empty() && all_of(s.begin(), s.end(), [](unsigned char c) { return isdigit(c); });
}

static bool isLegalId(const string &s, size_t maxLen) {
    if (s.empty() || s.size() > maxLen) return false;
    for (unsigned char c : s) {
        if (!(isalnum(c) || c == '_')) return false;
    }
    return true;
}

static bool isLegalText(const string &s, size_t maxLen, bool allowQuote = false, bool allowPipe = true) {
    if (s.empty() || s.size() > maxLen) return false;
    for (unsigned char c : s) {
        if (c < 33 || c == 127) return false;
        if (!allowQuote && c == '"') return false;
        if (!allowPipe && c == '|') return false;
    }
    return true;
}

static bool parseMoney(const string &s, i128 &value) {
    if (s.empty()) return false;
    size_t dot = s.find('.');
    string a = s, b;
    if (dot != string::npos) {
        if (s.find('.', dot + 1) != string::npos) return false;
        a = s.substr(0, dot);
        b = s.substr(dot + 1);
        if (b.size() > 2) return false;
    }
    if (a.empty() || !isDigits(a) || (!b.empty() && !isDigits(b))) return false;
    value = 0;
    for (char c : a) value = value * 10 + (c - '0');
    value *= 100;
    if (dot != string::npos) {
        if (b.size() == 1) b.push_back('0');
        if (b.empty()) b = "00";
        if (b.size() == 2) value += (b[0] - '0') * 10 + (b[1] - '0');
    }
    return true;
}

static string moneyToString(i128 value) {
    bool negative = value < 0;
    if (negative) value = -value;
    i128 whole = value / 100;
    int frac = static_cast<int>(value % 100);
    string result;
    do {
        result.push_back(char('0' + whole % 10));
        whole /= 10;
    } while (whole > 0);
    if (negative) result.push_back('-');
    reverse(result.begin(), result.end());
    result.push_back('.');
    result.push_back(char('0' + frac / 10));
    result.push_back(char('0' + frac % 10));
    return result;
}

class Bookstore {
public:
    Bookstore() { loadAll(); }
    ~Bookstore() { saveAll(); }

    void run() {
        string line;
        while (std::getline(cin, line)) {
            if (!processLine(line)) break;
        }
        saveAll();
    }

private:
    unordered_map<string, Account> accounts;
    unordered_map<string, Book> books;
    unordered_map<string, set<string>> nameIndex;
    unordered_map<string, set<string>> authorIndex;
    unordered_map<string, set<string>> keywordIndex;
    vector<LoginFrame> loginStack;
    unordered_map<string, int> activeSessions;
    vector<Transaction> transactions;
    vector<string> operationLogs;
    unordered_map<string, EmployeeStat> employeeStats;
    i128 totalIncome = 0;
    i128 totalExpenditure = 0;

    void loadAll() {
        loadAccounts();
        loadBooks();
        loadTransactions();
    }

    void saveAll() {
        saveAccounts();
        saveBooks();
        saveTransactions();
    }

    void loadAccounts() {
        ifstream in(ACCOUNTS_FILE);
        if (!in) {
            accounts["root"] = Account{"sjtu", "root", 7};
            return;
        }
        string line;
        while (getline(in, line)) {
            if (line.empty()) continue;
            auto parts = splitTab(line);
            if (parts.size() < 4) continue;
            accounts[parts[0]] = Account{parts[1], parts[3], stoi(parts[2])};
        }
        if (!accounts.count("root")) accounts["root"] = Account{"sjtu", "root", 7};
    }

    void saveAccounts() {
        ofstream out(ACCOUNTS_FILE, ios::trunc);
        vector<string> ids;
        ids.reserve(accounts.size());
        for (const auto &p : accounts) ids.push_back(p.first);
        sort(ids.begin(), ids.end());
        for (const auto &id : ids) {
            const auto &a = accounts[id];
            out << id << '\t' << a.password << '\t' << a.privilege << '\t' << a.username << '\n';
        }
    }

    void loadBooks() {
        ifstream in(BOOKS_FILE);
        if (!in) return;
        string line;
        while (getline(in, line)) {
            if (line.empty()) continue;
            auto parts = splitTab(line);
            if (parts.size() < 6) continue;
            Book b;
            b.isbn = parts[0];
            b.name = parts[1];
            b.author = parts[2];
            b.keywords = parts[3].empty() ? vector<string>{} : splitKeywords(parts[3]);
            b.price = 0;
            parseMoney(parts[4], b.price);
            b.stock = stoi(parts[5]);
            addIndexes(b);
            books[b.isbn] = std::move(b);
        }
    }

    void saveBooks() {
        ofstream out(BOOKS_FILE, ios::trunc);
        vector<string> ids;
        ids.reserve(books.size());
        for (const auto &p : books) ids.push_back(p.first);
        sort(ids.begin(), ids.end());
        for (const auto &id : ids) {
            const auto &b = books[id];
            out << b.isbn << '\t' << b.name << '\t' << b.author << '\t' << joinKeywords(b.keywords)
                << '\t' << moneyToString(b.price) << '\t' << b.stock << '\n';
        }
    }

    void loadTransactions() {
        ifstream in(TRANSACTIONS_FILE);
        if (!in) return;
        string line;
        while (getline(in, line)) {
            if (line.empty()) continue;
            auto parts = splitTab(line);
            if (parts.size() < 3) continue;
            Transaction t;
            t.type = parts[0];
            parseMoney(parts[1], t.income);
            parseMoney(parts[2], t.expenditure);
            transactions.push_back(t);
            totalIncome += t.income;
            totalExpenditure += t.expenditure;
        }
    }

    void saveTransactions() {
        ofstream out(TRANSACTIONS_FILE, ios::trunc);
        for (const auto &t : transactions) {
            out << t.type << '\t' << moneyToString(t.income) << '\t' << moneyToString(t.expenditure) << '\n';
        }
    }

    void addIndexes(const Book &book) {
        if (!book.name.empty()) nameIndex[book.name].insert(book.isbn);
        if (!book.author.empty()) authorIndex[book.author].insert(book.isbn);
        for (const auto &kw : book.keywords) {
            if (!kw.empty()) keywordIndex[kw].insert(book.isbn);
        }
    }

    void removeIndexes(const Book &book) {
        if (!book.name.empty()) {
            auto it = nameIndex.find(book.name);
            if (it != nameIndex.end()) {
                it->second.erase(book.isbn);
                if (it->second.empty()) nameIndex.erase(it);
            }
        }
        if (!book.author.empty()) {
            auto it = authorIndex.find(book.author);
            if (it != authorIndex.end()) {
                it->second.erase(book.isbn);
                if (it->second.empty()) authorIndex.erase(it);
            }
        }
        for (const auto &kw : book.keywords) {
            auto it = keywordIndex.find(kw);
            if (it != keywordIndex.end()) {
                it->second.erase(book.isbn);
                if (it->second.empty()) keywordIndex.erase(it);
            }
        }
    }

    int currentPrivilege() const {
        if (loginStack.empty()) return 0;
        auto it = accounts.find(loginStack.back().userId);
        return it == accounts.end() ? 0 : it->second.privilege;
    }

    string currentUser() const {
        return loginStack.empty() ? string() : loginStack.back().userId;
    }

    Book *currentBook() {
        if (loginStack.empty()) return nullptr;
        auto &frame = loginStack.back();
        if (frame.selectedIsbn.empty()) return nullptr;
        auto it = books.find(frame.selectedIsbn);
        return it == books.end() ? nullptr : &it->second;
    }

    void recordSuccess(const string &commandName) {
        string user = currentUser();
        if (!user.empty()) {
            employeeStats[user].commands++;
        }
        operationLogs.push_back((user.empty() ? string("guest") : user) + "\t" + commandName);
    }

    static bool parseQuotedOption(const string &token, const string &prefix, string &value) {
        if (token.size() < prefix.size() + 2) return false;
        if (token.compare(0, prefix.size(), prefix) != 0) return false;
        if (token[prefix.size()] != '"' || token.back() != '"') return false;
        value = token.substr(prefix.size() + 1, token.size() - prefix.size() - 2);
        return true;
    }

    static bool parseUnquotedOption(const string &token, const string &prefix, string &value) {
        if (token.compare(0, prefix.size(), prefix) != 0) return false;
        value = token.substr(prefix.size());
        return true;
    }

    bool ensurePrivilege(int required) const { return currentPrivilege() >= required; }

    bool processLine(string line) {
        string original = line;
        line = trim(line);
        if (line.empty()) return true;
        vector<string> tokens = tokenizeCommand(line);
        if (tokens.empty()) return true;
        const string &cmd = tokens[0];
        if (cmd == "quit" || cmd == "exit") {
            return false;
        }
        if (cmd == "su") return cmdSu(tokens, original);
        if (cmd == "logout") return cmdLogout(tokens);
        if (cmd == "register") return cmdRegister(tokens, line);
        if (cmd == "passwd") return cmdPasswd(tokens, line);
        if (cmd == "useradd") return cmdUseradd(tokens, line);
        if (cmd == "delete") return cmdDelete(tokens);
        if (cmd == "show" && tokens.size() >= 2 && tokens[1] == "finance") return cmdShowFinance(tokens);
        if (cmd == "show") return cmdShow(tokens, line);
        if (cmd == "buy") return cmdBuy(tokens);
        if (cmd == "select") return cmdSelect(tokens);
        if (cmd == "modify") return cmdModify(tokens, line);
        if (cmd == "import") return cmdImport(tokens);
        if (cmd == "log") return cmdLog(tokens);
        if (cmd == "report") return cmdReport(tokens);
        invalid();
        return true;
    }

    void invalid() { cout << "Invalid\n"; }

    bool cmdSu(const vector<string> &tokens, const string &) {
        if (tokens.size() != 2 && tokens.size() != 3) return invalidRet();
        const string &userId = tokens[1];
        if (!isLegalId(userId, 30)) return invalidRet();
        auto it = accounts.find(userId);
        if (it == accounts.end()) return invalidRet();
        if (tokens.size() == 2) {
            if (currentPrivilege() <= it->second.privilege) return invalidRet();
        } else {
            if (tokens[2] != it->second.password) return invalidRet();
        }
        loginStack.push_back(LoginFrame{userId, string()});
        ++activeSessions[userId];
        recordSuccess("su");
        return true;
    }

    bool cmdLogout(const vector<string> &tokens) {
        if (tokens.size() != 1) return invalidRet();
        if (loginStack.empty()) return invalidRet();
        string user = loginStack.back().userId;
        loginStack.pop_back();
        auto it = activeSessions.find(user);
        if (it != activeSessions.end()) {
            if (--it->second == 0) activeSessions.erase(it);
        }
        recordSuccess("logout");
        return true;
    }

    bool cmdRegister(const vector<string> &tokens, const string &line) {
        auto spans = tokenizeWithSpans(line);
        if (spans.size() < 4) return invalidRet();
        const string &userId = tokens[1];
        const string &password = tokens[2];
        string username = line.substr(spans[3].start);
        if (!isLegalId(userId, 30) || !isLegalId(password, 30) || !isLegalText(username, 30, false, true)) return invalidRet();
        if (accounts.count(userId)) return invalidRet();
        accounts[userId] = Account{password, username, 1};
        recordSuccess("register");
        return true;
    }

    bool cmdPasswd(const vector<string> &tokens, const string &) {
        if (tokens.size() != 3 && tokens.size() != 4) return invalidRet();
        if (!isLegalId(tokens[1], 30) || !isLegalId(tokens.back(), 30)) return invalidRet();
        auto it = accounts.find(tokens[1]);
        if (it == accounts.end()) return invalidRet();
        if (tokens.size() == 3) {
            if (currentPrivilege() != 7) return invalidRet();
        } else {
            if (tokens[2] != it->second.password) return invalidRet();
        }
        it->second.password = tokens.back();
        recordSuccess("passwd");
        return true;
    }

    bool cmdUseradd(const vector<string> &tokens, const string &line) {
        auto spans = tokenizeWithSpans(line);
        if (spans.size() < 5) return invalidRet();
        if (!ensurePrivilege(3)) return invalidRet();
        const string &userId = tokens[1];
        const string &password = tokens[2];
        const string &privText = tokens[3];
        string username = line.substr(spans[4].start);
        if (!isLegalId(userId, 30) || !isLegalId(password, 30) || privText.size() != 1 || !isdigit(privText[0]) || !isLegalText(username, 30, false, true)) return invalidRet();
        int privilege = privText[0] - '0';
        if (!(privilege == 1 || privilege == 3) || privilege >= currentPrivilege()) return invalidRet();
        if (accounts.count(userId)) return invalidRet();
        accounts[userId] = Account{password, username, privilege};
        recordSuccess("useradd");
        return true;
    }

    bool cmdDelete(const vector<string> &tokens) {
        if (tokens.size() != 2) return invalidRet();
        if (!ensurePrivilege(7)) return invalidRet();
        const string &userId = tokens[1];
        if (!isLegalId(userId, 30)) return invalidRet();
        auto it = accounts.find(userId);
        if (it == accounts.end()) return invalidRet();
        if (activeSessions.count(userId)) return invalidRet();
        accounts.erase(it);
        recordSuccess("delete");
        return true;
    }

    bool cmdSelect(const vector<string> &tokens) {
        if (tokens.size() != 2) return invalidRet();
        if (!ensurePrivilege(3)) return invalidRet();
        if (!isLegalText(tokens[1], 20, true, true)) return invalidRet();
        Book &book = books[tokens[1]];
        if (book.isbn.empty()) {
            book.isbn = tokens[1];
            addIndexes(book);
        }
        loginStack.back().selectedIsbn = tokens[1];
        recordSuccess("select");
        return true;
    }

    bool cmdBuy(const vector<string> &tokens) {
        if (tokens.size() != 3) return invalidRet();
        if (!ensurePrivilege(1)) return invalidRet();
        if (!isLegalText(tokens[1], 20, true, true) || !isDigits(tokens[2])) return invalidRet();
        long long qty = 0;
        for (char c : tokens[2]) qty = qty * 10 + (c - '0');
        if (qty <= 0 || qty > 2147483647LL) return invalidRet();
        auto it = books.find(tokens[1]);
        if (it == books.end() || it->second.stock < qty) return invalidRet();
        it->second.stock -= static_cast<int>(qty);
        i128 amount = it->second.price * qty;
        totalIncome += amount;
        transactions.push_back(Transaction{amount, 0, "buy", tokens[1], static_cast<int>(qty)});
        cout << moneyToString(amount) << '\n';
        recordSuccess("buy");
        return true;
    }

    bool cmdModify(const vector<string> &tokens, const string &) {
        if (tokens.size() < 2) return invalidRet();
        if (!ensurePrivilege(3)) return invalidRet();
        Book *book = currentBook();
        if (!book) return invalidRet();
        unordered_set<string> seen;
        for (size_t i = 1; i < tokens.size(); ++i) {
            const string &token = tokens[i];
            string value;
            if (token.rfind("-ISBN=", 0) == 0) {
                if (!seen.insert("ISBN").second) return invalidRet();
                value = token.substr(6);
                if (!isLegalText(value, 20, true, true) || value == book->isbn || books.count(value)) return invalidRet();
                Book old = *book;
                removeIndexes(*book);
                books.erase(old.isbn);
                old.isbn = value;
                books[value] = old;
                book = &books[value];
                addIndexes(*book);
                loginStack.back().selectedIsbn = value;
            } else if (parseQuotedOption(token, "-name=", value)) {
                if (!seen.insert("name").second) return invalidRet();
                if (!isLegalText(value, 60, false, true)) return invalidRet();
                removeIndexes(*book);
                book->name = value;
                addIndexes(*book);
            } else if (parseQuotedOption(token, "-author=", value)) {
                if (!seen.insert("author").second) return invalidRet();
                if (!isLegalText(value, 60, false, true)) return invalidRet();
                removeIndexes(*book);
                book->author = value;
                addIndexes(*book);
            } else if (parseQuotedOption(token, "-keyword=", value)) {
                if (!seen.insert("keyword").second) return invalidRet();
                if (!isLegalText(value, 60, false, true)) return invalidRet();
                auto kws = splitKeywords(value);
                if (!keywordListValid(kws)) return invalidRet();
                removeIndexes(*book);
                book->keywords = std::move(kws);
                addIndexes(*book);
            } else if (token.rfind("-price=", 0) == 0) {
                if (!seen.insert("price").second) return invalidRet();
                value = token.substr(7);
                i128 price = 0;
                if (!parseMoney(value, price)) return invalidRet();
                book->price = price;
            } else {
                return invalidRet();
            }
        }
        recordSuccess("modify");
        return true;
    }

    bool cmdImport(const vector<string> &tokens) {
        if (tokens.size() != 3) return invalidRet();
        if (!ensurePrivilege(3)) return invalidRet();
        Book *book = currentBook();
        if (!book) return invalidRet();
        if (!isDigits(tokens[1])) return invalidRet();
        i128 cost = 0;
        if (!parseMoney(tokens[2], cost) || cost <= 0) return invalidRet();
        long long qty = 0;
        for (char c : tokens[1]) qty = qty * 10 + (c - '0');
        if (qty <= 0) return invalidRet();
        book->stock += static_cast<int>(qty);
        totalExpenditure += cost;
        transactions.push_back(Transaction{0, cost, "import", book->isbn, static_cast<int>(qty)});
        recordSuccess("import");
        return true;
    }

    bool cmdShow(const vector<string> &tokens, const string &line) {
        if (tokens.size() == 1) {
            if (!ensurePrivilege(1)) return invalidRet();
            printBooks(collectAllBooks());
            recordSuccess("show");
            return true;
        }
        if (tokens.size() != 2) return invalidRet();
        if (tokens[1].rfind("-ISBN=", 0) == 0) {
            if (!ensurePrivilege(1)) return invalidRet();
            string isbn = tokens[1].substr(6);
            if (!isLegalText(isbn, 20, true, true) || isbn.empty()) return invalidRet();
            vector<const Book *> res;
            auto it = books.find(isbn);
            if (it != books.end()) res.push_back(&it->second);
            printBooks(res);
            recordSuccess("show");
            return true;
        }
        if (tokens[1].rfind("-name=", 0) == 0 || tokens[1].rfind("-author=", 0) == 0 || tokens[1].rfind("-keyword=", 0) == 0) {
            if (!ensurePrivilege(1)) return invalidRet();
            string value;
            vector<const Book *> res;
            if (parseQuotedOption(tokens[1], "-name=", value)) {
                if (!isLegalText(value, 60, false, true) || value.empty()) return invalidRet();
                auto it = nameIndex.find(value);
                if (it != nameIndex.end()) collectByIsbns(it->second, res);
            } else if (parseQuotedOption(tokens[1], "-author=", value)) {
                if (!isLegalText(value, 60, false, true) || value.empty()) return invalidRet();
                auto it = authorIndex.find(value);
                if (it != authorIndex.end()) collectByIsbns(it->second, res);
            } else if (parseQuotedOption(tokens[1], "-keyword=", value)) {
                if (!isLegalText(value, 60, false, true) || value.empty() || value.find('|') != string::npos) return invalidRet();
                auto it = keywordIndex.find(value);
                if (it != keywordIndex.end()) collectByIsbns(it->second, res);
            } else {
                return invalidRet();
            }
            printBooks(res);
            recordSuccess("show");
            return true;
        }
        return invalidRet();
    }

    bool cmdShowFinance(const vector<string> &tokens) {
        if (!ensurePrivilege(7)) return invalidRet();
        if (tokens.size() == 2) {
            if (tokens[1] != "finance") return invalidRet();
            cout << "+ " << moneyToString(totalIncome) << " - " << moneyToString(totalExpenditure) << '\n';
            recordSuccess("show finance");
            return true;
        }
        if (tokens.size() == 3 && tokens[1] == "finance") {
            if (!isDigits(tokens[2])) return invalidRet();
            long long cnt = 0;
            for (char c : tokens[2]) cnt = cnt * 10 + (c - '0');
            if (cnt > 2147483647LL) return invalidRet();
            if (cnt == 0) {
                cout << '\n';
                recordSuccess("show finance");
                return true;
            }
            if (cnt > static_cast<long long>(transactions.size())) return invalidRet();
            i128 income = 0, expenditure = 0;
            for (long long i = transactions.size() - cnt; i < static_cast<long long>(transactions.size()); ++i) {
                income += transactions[i].income;
                expenditure += transactions[i].expenditure;
            }
            cout << "+ " << moneyToString(income) << " - " << moneyToString(expenditure) << '\n';
            recordSuccess("show finance");
            return true;
        }
        return invalidRet();
    }

    bool cmdLog(const vector<string> &tokens) {
        if (tokens.size() != 1) return invalidRet();
        if (!ensurePrivilege(7)) return invalidRet();
        cout << "Operations:\n";
        for (const auto &s : operationLogs) cout << s << '\n';
        cout << "Transactions:\n";
        for (const auto &t : transactions) {
            cout << t.type << '\t' << moneyToString(t.income) << '\t' << moneyToString(t.expenditure) << '\t' << t.isbn << '\t' << t.quantity << '\n';
        }
        recordSuccess("log");
        return true;
    }

    bool cmdReport(const vector<string> &tokens) {
        if (tokens.size() != 2) return invalidRet();
        if (!ensurePrivilege(7)) return invalidRet();
        if (tokens[1] == "finance") {
            cout << "Finance Report\n";
            cout << "Income\t" << moneyToString(totalIncome) << '\n';
            cout << "Expenditure\t" << moneyToString(totalExpenditure) << '\n';
            cout << "Transactions\t" << transactions.size() << '\n';
            recordSuccess("report finance");
            return true;
        }
        if (tokens[1] == "employee") {
            cout << "Employee Report\n";
            vector<string> ids;
            for (const auto &p : accounts) if (p.second.privilege >= 3) ids.push_back(p.first);
            sort(ids.begin(), ids.end());
            for (const auto &id : ids) {
                cout << id << '\t' << accounts[id].privilege << '\t' << employeeStats[id].commands << '\n';
            }
            recordSuccess("report employee");
            return true;
        }
        return invalidRet();
    }

    bool invalidRet() {
        invalid();
        return true;
    }

    vector<const Book *> collectAllBooks() const {
        vector<const Book *> res;
        res.reserve(books.size());
        for (const auto &p : books) res.push_back(&p.second);
        sort(res.begin(), res.end(), [](const Book *a, const Book *b) { return a->isbn < b->isbn; });
        return res;
    }

    void collectByIsbns(const set<string> &isbns, vector<const Book *> &res) const {
        for (const auto &isbn : isbns) {
            auto it = books.find(isbn);
            if (it != books.end()) res.push_back(&it->second);
        }
    }

    void printBooks(const vector<const Book *> &res) const {
        if (res.empty()) {
            cout << '\n';
            return;
        }
        vector<const Book *> sorted = res;
        sort(sorted.begin(), sorted.end(), [](const Book *a, const Book *b) { return a->isbn < b->isbn; });
        for (const Book *b : sorted) {
            cout << b->isbn << '\t' << b->name << '\t' << b->author << '\t' << joinKeywords(b->keywords)
                 << '\t' << moneyToString(b->price) << '\t' << b->stock << '\n';
        }
    }
};

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);
    Bookstore app;
    app.run();
    return 0;
}
