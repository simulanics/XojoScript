// PluginHTMLtoMarkdown.cpp
// Build with g++ -shared -fPIC -lcurl -o libHTMLtoMarkdown.so PluginHTMLtoMarkdown.cpp
// Requires libcurl


#include <cstdio>
#include <string>
#include <curl/curl.h>
#include <regex>
#include <unordered_map>
#include <iostream>

#ifdef _WIN32
#include <windows.h>
#define XPLUGIN_API __declspec(dllexport)
#else
#define XPLUGIN_API __attribute__((visibility("default")))
#endif

extern "C" {

// Callback for libcurl to write fetched data into a std::string.
size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t totalSize = size * nmemb;
    std::string* str = static_cast<std::string*>(userp);
    str->append(static_cast<char*>(contents), totalSize);
    return totalSize;
}

// Convert HTML to Markdown via basic tag replacements.
std::string htmlToMarkdownImpl(const std::string &html) {
    std::string md = html;
    md = std::regex_replace(md, std::regex("<script[\\s\\S]*?</script>", std::regex_constants::icase), "");
    md = std::regex_replace(md, std::regex("<style[\\s\\S]*?</style>", std::regex_constants::icase), "");
    md = std::regex_replace(md, std::regex("<br\\s*/?>", std::regex_constants::icase), "\n");
    md = std::regex_replace(md, std::regex("<h1>(.*?)</h1>", std::regex_constants::icase), "# $1\n\n");
    md = std::regex_replace(md, std::regex("<h2>(.*?)</h2>", std::regex_constants::icase), "## $1\n\n");
    md = std::regex_replace(md, std::regex("<h3>(.*?)</h3>", std::regex_constants::icase), "### $1\n\n");
    md = std::regex_replace(md, std::regex("<h4>(.*?)</h4>", std::regex_constants::icase), "#### $1\n\n");
    md = std::regex_replace(md, std::regex("<h5>(.*?)</h5>", std::regex_constants::icase), "##### $1\n\n");
    md = std::regex_replace(md, std::regex("<h6>(.*?)</h6>", std::regex_constants::icase), "###### $1\n\n");
    md = std::regex_replace(md, std::regex("<p>(.*?)</p>", std::regex_constants::icase), "$1\n\n");
    md = std::regex_replace(md, std::regex("<(strong|b)>(.*?)</\\1>", std::regex_constants::icase), "**$2**");
    md = std::regex_replace(md, std::regex("<(em|i)>(.*?)</\\1>", std::regex_constants::icase), "*$2*");
    md = std::regex_replace(md, std::regex("<li>(.*?)</li>", std::regex_constants::icase), "- $1\n");
    md = std::regex_replace(md, std::regex("</?(ul|ol)>", std::regex_constants::icase), "");
    md = std::regex_replace(md, std::regex("<[^>]*>"), "");
    return md;
}

std::string collapseNewlines(const std::string &text) {
    return std::regex_replace(text, std::regex("((\\n\\s*){2,})"), "\n\n");
}

std::string decodeHtmlEntities(const std::string &text) {
    std::string result;
    size_t lastPos = 0;
    std::regex entityRegex("(&#(\\d+);)|(&([a-zA-Z]+);)");
    auto begin = std::sregex_iterator(text.begin(), text.end(), entityRegex);
    auto end = std::sregex_iterator();
    static const std::unordered_map<std::string, std::string> entityMap = {
        {"bull", "•"}, {"amp", "&"}, {"lt", "<"}, {"gt", ">"}, {"quot", "\""},
        {"apos", "'"}, {"copy", "©"}, {"nbsp", " "}, {"ndash", "–"}, {"mdash", "—"},
        {"lsquo", "‘"}, {"rsquo", "’"}, {"ldquo", "“"}, {"rdquo", "”"}, {"hellip", "…"}
    };

    for (auto it = begin; it != end; ++it) {
        std::smatch match = *it;
        result.append(text.substr(lastPos, match.position() - lastPos));
        if (match[2].matched) {
            int code = std::stoi(match[2].str());
            if (code == 8217)
                result.push_back('\'');
            else if (code == 8220 || code == 8221)
                result.push_back('\"');
            else if (code == 8211)
                result.push_back('-');
            else if (code < 128)
                result.push_back(static_cast<char>(code));
            else
                result.append(match.str());
        } else if (match[4].matched) {
            std::string name = match[4].str();
            auto found = entityMap.find(name);
            if (found != entityMap.end())
                result.append(found->second);
            else
                result.append(match.str());
        }
        lastPos = match.position() + match.length();
    }
    result.append(text.substr(lastPos));
    return result;
}

// Exported function: converts HTML to Markdown.
// Accepts a const char* and returns a const char* (like sayhello).
XPLUGIN_API const char* htmltomarkdown(const char* html) {
    static std::string result;
    std::string input(html);
    result = htmlToMarkdownImpl(input);
    result = collapseNewlines(result);
    result = decodeHtmlEntities(result);
    return result.c_str();
}

// Exported function: fetches HTML from a URL and converts it to Markdown.
// Name matches the style of sayhello.
XPLUGIN_API const char* urltomarkdown(const char* url) {
    static std::string result;
    std::string htmlContent;
    CURL* curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (compatible; MyApp/1.0; +http://example.com/)");
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &htmlContent);
        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            result = "Error fetching URL";
            curl_easy_cleanup(curl);
            return result.c_str();
        }
        curl_easy_cleanup(curl);
    } else {
        result = "Failed to initialize curl";
        return result.c_str();
    }
    result = htmlToMarkdownImpl(htmlContent);
    result = collapseNewlines(result);
    result = decodeHtmlEntities(result);
    return result.c_str();
}

// PluginEntry structure.
typedef struct PluginEntry {
    const char* name;    // Name to be used by the interpreter.
    void* funcPtr;       // Pointer to the exported function.
    int arity;           // Number of parameters.
    const char* paramTypes[10]; // Parameter datatypes.
    const char* retType; // Return datatype.
} PluginEntry;

// Plugin entries.
static PluginEntry pluginEntries[] = {
    { "htmltomarkdown", (void*)htmltomarkdown, 1, { "string" }, "string" },
    { "urltomarkdown", (void*)urltomarkdown, 1, { "string" }, "string" }
};

// Exported function to retrieve plugin entries.
XPLUGIN_API PluginEntry* GetPluginEntries(int* count) {
    if (count) {
        *count = sizeof(pluginEntries) / sizeof(PluginEntry);
    }
    return pluginEntries;
}

} // extern "C"

#ifdef _WIN32
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}
#endif

