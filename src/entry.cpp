#include <entry.hpp>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace {
    std::string escapeField(const std::string& value)
    {
        std::string out;
        out.reserve(value.size());

        for (char c : value) {
            switch (c) {
                case '\\': out += "\\\\"; break;
                case '|': out += "\\|"; break;
                case '\n': out += "\\n"; break;
                case '\r': out += "\\r"; break;
                default: out += c; break;
            }
        }

        return out;
    }

    std::vector<std::string> splitEscaped(const std::string& line)
    {
        std::vector<std::string> fields;
        std::string current;
        bool escaping = false;

        for (char c : line) {
            if (escaping) {
                switch (c) {
                    case 'n': current += '\n'; break;
                    case 'r': current += '\r'; break;
                    default: current += c; break;
                }
                escaping = false;
                continue;
            }

            if (c == '\\') {
                escaping = true;
                continue;
            }

            if (c == '|') {
                fields.push_back(current);
                current.clear();
                continue;
            }

            current += c;
        }

        if (escaping) {
            current += '\\';
        }

        fields.push_back(current);
        return fields;
    }
}

std::string Entry::serialize() const
{
    return std::to_string(id) + "|" +
           escapeField(website) + "|" +
           escapeField(username) + "|" +
           escapeField(password) + "|" +
           escapeField(note);
}

Entry Entry::deserialize(const std::string& line)
{
    std::vector<std::string> fields = splitEscaped(line);
    if (fields.size() != 5) {
        throw std::runtime_error("Invalid entry format");
    }

    int id = std::stoi(fields[0]);
    return Entry(id, fields[1], fields[2], fields[3], fields[4]);
}

std::string Entry::toString() const {
    std::ostringstream out;
    out << "Id: " << id << " | " << " Website: " << " | " << website << " | " << "Username: " << username << " | " << "Password: " << password << " | " << "Note: " << note;
    return out.str();
}