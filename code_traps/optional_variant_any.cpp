#include <any>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>
using namespace std;

/*
 * Topic:
 * std::optional, std::variant, std::any.
 *
 * Compile:
 * g++ -std=c++17 optional_variant_any.cpp -o optional_variant_any
 *
 * Run:
 * ./optional_variant_any
 */

optional<int> findIndex(const vector<int>& values, int target) {
    for (size_t i = 0; i < values.size(); ++i) {
        if (values[i] == target) {
            return static_cast<int>(i);
        }
    }

    return nullopt;
}

optional<int> parsePositiveInt(string_view s) {
    if (s.empty()) {
        return nullopt;
    }

    int value = 0;

    for (char c : s) {
        if (c < '0' || c > '9') {
            return nullopt;
        }

        value = value * 10 + (c - '0');
    }

    return value;
}

void optionalDemo() {
    cout << "\n=== optional demo ===" << endl;

    vector<int> values = {10, 20, 30};

    auto idx = findIndex(values, 20);

    if (idx) {
        cout << "found at index " << *idx << endl;
    } else {
        cout << "not found\n";
    }

    auto missing = findIndex(values, 99);

    cout << "missing value_or(-1) = " << missing.value_or(-1) << endl;

    auto parsed = parsePositiveInt("123");

    if (parsed.has_value()) {
        cout << "parsed = " << parsed.value() << endl;
    }

    auto bad = parsePositiveInt("12x");

    if (!bad) {
        cout << "parse failed\n";
    }
}

class Tracker {
private:
    string name;

public:
    explicit Tracker(string n)
        : name(std::move(n)) {
        cout << "Tracker constructor: " << name << endl;
    }

    ~Tracker() {
        cout << "Tracker destructor: " << name << endl;
    }

    Tracker(const Tracker& other)
        : name(other.name + "_copy") {
        cout << "Tracker copy constructor from " << other.name << endl;
    }

    Tracker(Tracker&& other) noexcept
        : name(std::move(other.name)) {
        cout << "Tracker move constructor" << endl;
        other.name = "moved_from";
    }
};

void optionalLifetimeDemo() {
    cout << "\n=== optional lifetime demo ===" << endl;

    optional<Tracker> opt;

    cout << "empty optional created, no Tracker constructed yet\n";

    opt.emplace("inside_optional");

    cout << "reset optional now\n";
    opt.reset();

    cout << "optional reset destroyed contained Tracker\n";
}

struct ParseError {
    string message;
};

using ParseResult = variant<int, ParseError>;

ParseResult parseIntDetailed(string_view s) {
    if (s.empty()) {
        return ParseError{"empty input"};
    }

    int value = 0;

    for (char c : s) {
        if (c < '0' || c > '9') {
            return ParseError{"non-digit character"};
        }

        value = value * 10 + (c - '0');
    }

    return value;
}

void variantDemo() {
    cout << "\n=== variant demo ===" << endl;

    variant<int, string> v;

    v = 42;

    if (holds_alternative<int>(v)) {
        cout << "v holds int: " << get<int>(v) << endl;
    }

    v = string("hello");

    if (auto p = get_if<string>(&v)) {
        cout << "v holds string: " << *p << endl;
    }

    visit([](const auto& value) {
        cout << "visit active value: " << value << endl;
    }, v);
}

void parseResultVariantDemo() {
    cout << "\n=== parse result variant demo ===" << endl;

    for (string_view input : {"123", "", "12x"}) {
        ParseResult r = parseIntDetailed(input);

        cout << "input [" << input << "]: ";

        if (auto value = get_if<int>(&r)) {
            cout << "value = " << *value << endl;
        } else if (auto err = get_if<ParseError>(&r)) {
            cout << "error = " << err->message << endl;
        }
    }
}

void monostateDemo() {
    cout << "\n=== monostate demo ===" << endl;

    variant<monostate, int, string> v;

    if (holds_alternative<monostate>(v)) {
        cout << "v is empty/monostate\n";
    }

    v = 10;

    cout << "v now holds int: " << get<int>(v) << endl;
}

void anyDemo() {
    cout << "\n=== any demo ===" << endl;

    any a;

    a = 42;

    cout << "a holds int: " << any_cast<int>(a) << endl;

    a = string("hello any");

    if (auto p = any_cast<string>(&a)) {
        cout << "a holds string: " << *p << endl;
    }

    try {
        cout << any_cast<int>(a) << endl;
    } catch (const bad_any_cast& e) {
        cout << "bad_any_cast caught\n";
    }
}

void anyVsVoidPointerDemo() {
    cout << "\n=== any vs void* demo ===" << endl;

    cout << "std::any remembers the stored type and checks any_cast." << endl;
    cout << "void* is just an address and does not remember the type." << endl;
}

int main() {
    optionalDemo();

    optionalLifetimeDemo();

    variantDemo();

    parseResultVariantDemo();

    monostateDemo();

    anyDemo();

    anyVsVoidPointerDemo();

    cout << "\n=== end of main ===" << endl;

    return 0;
}