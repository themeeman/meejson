# Meejson

Meejson is a JSON parsing and object manipulation library. 

- Ergonomic and simple to use API
- Customisable to a reasonable degree
- Zero dependencies
- Written purely in C++20, making the source code very easy to read (no preprocessor gibberish)

## Examples

```c++
#include "meejson/value.hpp"
#include "meejson/parser.hpp"

namespace json = mee::json;

int main() {
    auto val = json::parse("[3.5, 2.3, 1.0, 3, -1e2]");
    for (auto& x : val.get_array()) {
        x++;
    }
    std::ranges::sort(val.get_array());
    std::cout << val; // Prints out [0.99, 2.0, 3.3, 4, 4.5]
}
```

Replacing missing fields with null

```c++
#include "meejson/value.hpp"

namespace json = mee::json;

int main() { 
    auto data = R"([
    {
        "first_name": "Tom",
        "last_name": "Van Howe",
        "age": 18
    },
    {
        "first_name": "Bjarne",
        "last_name": "Stroutstrup"
    },
    {
        "first_name": "Jeff",
        "age": 50
    }
])"_json;
    auto keys = std::set<std::string>();
    for (const auto& obj : data.get_array()) {
        for (const auto& [key, val] : obj.get_object()) {
            keys.insert(key);
        }
    }
    
    for (auto& obj : data.get_array()) {
        for (const auto& key : keys) {
            if (!obj.has_key(key)) {
                obj.emplace(key, json::null());
            }   
        }   
    }
    std::cout << data;
}
```
