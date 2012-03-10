#if !defined(rc_config_h)
#define rc_config_h

#include <string>
#include <map>
#include <list>

struct Config
{
    std::string value_;
    std::map<std::string, Config> keys_;

    bool hasKey(std::string const &key) const
    {
        return keys_.find(key) != keys_.end();
    }
    bool hasString(std::string const &key) const
    {
        std::map<std::string, Config>::const_iterator ptr(keys_.find(key));
        if (ptr == keys_.end()) return false;
        //  The empty string is OK, as long as it doesn't have sub-values
        return (*ptr).second.keys_.size() == 0;
    }
    bool hasNumber(std::string const &key) const
    {
        std::map<std::string, Config>::const_iterator ptr(keys_.find(key));
        if (ptr == keys_.end()) return false;
        char *end = 0;
        char const *str = (*ptr).second.value_.c_str();
        strtod(str, &end);
        return end != 0 && end > str;
    }
    bool hasSubkeys(std::string const &key) const
    {
        std::map<std::string, Config>::const_iterator ptr(keys_.find(key));
        if (ptr == keys_.end()) return false;
        return (*ptr).second.keys_.size() > 0;
    }

    std::string const &string(std::string const &key) const
    {
        std::map<std::string, Config>::const_iterator ptr(keys_.find(key));
        if (ptr == keys_.end()) goto error;
        if ((*ptr).second.keys_.size() > 0)
        {
    error:
            throw std::runtime_error(std::string("No string value for key ") + key);
        }
        //  The empty string is OK, as long as it doesn't have sub-values
        return (*ptr).second.value_;
    }
    double number(std::string const &key) const
    {
        std::map<std::string, Config>::const_iterator ptr(keys_.find(key));
        if (ptr == keys_.end()) goto error;
        char *end = 0;
        char const *str = (*ptr).second.value_.c_str();
        double ret = strtod(str, &end);
        if (end == 0 || end <= str)
        {
    error:
            throw std::runtime_error(std::string("No number value for key ") + key);
        }
        return ret;
    }
    float numberf(std::string const &key) const
    {
        return (float)number(key);
    }
    Config const &subkeys(std::string const &key) const
    {
        std::map<std::string, Config>::const_iterator ptr(keys_.find(key));
        if (ptr == keys_.end()) goto error;
        if ((*ptr).second.keys_.size() == 0)
        {
    error:
            throw std::runtime_error(std::string("No sub-keys value for key ") + key);
        }
        return (*ptr).second;
    }
    Config const &operator[](std::string const &key) const
    {
        return subkeys(key);
    }
    std::list<std::string> keys() const
    {
        if (keys_.size() == 0)
        {
            throw std::runtime_error(std::string("No sub-keys value in keys()"));
        }
        std::list<std::string> ret;
        for (std::map<std::string, Config>::const_iterator ptr(keys_.begin()), end(keys_.end());
            ptr != end; ++ptr)
        {
            ret.push_back((*ptr).first);
        }
        return ret;
    }
};


#endif  //  rc_config_h
