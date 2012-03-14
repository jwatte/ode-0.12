
#include "rc_scene.h"
#include "rc_ixfile.h"
#include "rc_gameobj.h"
#include "rc_config.h"
#include "rc_scenegraph.h"
#include "rc_ode.h"
#include "rc_debug.h"

#include <list>
#include <string>
#include <map>
#include <set>

bool sceneRunning = true;
std::set<GameObject *> sceneObjects;

void clearScene()
{
    std::set<GameObject *> tmp;
    tmp.swap(sceneObjects);
    for (std::set<GameObject *>::iterator ptr(tmp.begin()), end(tmp.end());
        ptr != end; ++ptr)
    {
        deleteSceneObject(*ptr);
    }
    physicsClear();
}

void assertInScene(GameObject *obj, bool inScene)
{
    bool isInScene = (sceneObjects.find(obj) != sceneObjects.end());
    if (isInScene != inScene)
    {
        throw std::runtime_error(inScene ? "Game object not in scene as expected" : "Game object is in scene, shouldn't be");
    }
}

void deleteSceneObject(GameObject *obj)
{
    std::set<GameObject *>::iterator ptr(sceneObjects.find(obj));
    if (ptr != sceneObjects.end())
    {
        sceneObjects.erase(ptr);
        obj->on_removeFromScene();
    }
    delete obj;
}

enum CharClass
{
    cc_null,
    cc_space,
    cc_identifier,
    cc_number,
    cc_numClasses
};

CharClass char_class(char ch, CharClass prev = cc_null)
{
    if (ch >= 'a' && ch <= 'z') return cc_identifier;
    if (ch >= 'A' && ch <= 'Z') return cc_identifier;
    if (ch == '_') return cc_identifier;
    if ((ch >= '0' && ch <= '9') || (ch == '.') || (ch == '/') || (ch == '-')) return (prev == cc_identifier) ? cc_identifier : cc_number;
    if (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r') return cc_space;
    return (CharClass)ch;
}

bool match_stanza(std::vector<std::pair<int, std::string> > &tokens, size_t &pos, Config &cfg)
{
    size_t size = tokens.size();
    if (pos >= tokens.size())
    {
        return true;
    }
    std::string &id(tokens[pos].second);
    ++pos;
    if (pos >= size)
    {
        return false;
    }
    if (cfg.hasKey(id))
    {
        //  duplicate assignment of key name
        return false;
    }
    std::string &assign(tokens[pos].second);
    ++pos;
    if (assign == "{")
    {
        Config subc;
        while (pos < size && tokens[pos].second != "}")
        {
            if (!match_stanza(tokens, pos, subc))
            {
                //  an error from below
                return false;
            }
        }
        if (pos == size)
        {
            //  unexpected end of file
            return false;
        }
        //  so, token[pos] must be closing '}'
        ++pos;
        cfg.keys_[id] = subc;
        return true;
    }
    else
    {
        //  it's a value
        cfg.keys_[id].value_ = assign;
    }
    //  allow trailing commas
    if (pos < size && tokens[pos].second == ",")
    {
        ++pos;
    }
    return true;
}

void addSceneObject(GameObject *gobj)
{
    sceneObjects.insert(gobj);
    gobj->on_addToScene();
}

void instantiateScene(Config &cfg)
{
    std::list<std::string> keys(cfg.keys());
    for (std::list<std::string>::iterator ptr(keys.begin()), end(keys.end());
        ptr != end; ++ptr)
    {
        Config const &obj = cfg[*ptr];
        std::string const &type = obj.string("type");
        GameObjectType *got = GameObjectType::type(type);
        GameObject *go = got->instantiate(obj);
        addSceneObject(go);
    }
}

void parseScene(char const *data, size_t size)
{
    Config config;
    int line = 1;
    char const *end = data + size;
    std::vector<std::pair<int, std::string>> tokens;
    while (data < end)
    {
        while (data < end && isspace(*data))
        {
            if (*data == '\n')
            {
                ++line;
            }
            ++data;
        }
        char const *start = data;
        if (start == end)
        {
            break;
        }
        if (*data == '\"')
        {
            //  parse string
            ++data;
            std::string ret;
            char const *start = data;
            while (data < end)
            {
                if (*data == '"')
                {
                    if (data < end - 1 && data[1] == '"')
                    {
                        ++data;
                        ret.insert(ret.end(), start, data);
                        start = data + 1;
                    }
                    else
                    {
                        //  I have my string
                        break;
                    }
                }
                ++data;
            }
            if (start < data)
            {
                ret.insert(ret.end(), start, data);
            }
            if (data < end)
            {
                ++data;
            }
        }
        else if (*data == '#')
        {
            //  parse comment
            while (data < end && *data != '\n')
            {
                ++data;
            }
        }
        else
        {
            CharClass cc = char_class(*data);
            while (data < end && char_class(*data, cc) == cc)
            {
                ++data;
            }
            tokens.push_back(std::pair<int, std::string>(line, std::string(start, data)));
        }
    }
    while (tokens.size() > 0)
    {
        size_t pos = 0;
        if (!match_stanza(tokens, pos, config) || pos == 0)
        {
            char str[1000];
            if (pos >= tokens.size())
            {
                //  I can only get here with non-empty "tokens"
                --pos;
            }
            _snprintf_s(str, 1000, "Error in config on line %d:\nStarting on line %d:\nNear %s", 
                tokens[pos].first, line, tokens[pos].second.c_str());
            throw std::runtime_error(std::string(str));
        }
        tokens.erase(tokens.begin(), tokens.begin() + pos);
    }
    instantiateScene(config);
}

void loadScene(char const *name)
{
    physicsCreate();
    IxRead *r = IxRead::readFromFile(name);
    parseScene((char const *)r->dataSegment(0, r->size()), r->size());
    delete r;
}

void stepScene()
{
    clearDebugLines();
    physicsStep();
    for (std::set<GameObject *>::iterator ptr(sceneObjects.begin()), end(sceneObjects.end());
        ptr != end; ++ptr)
    {
        (*ptr)->on_step();
    }
}

void renderScene()
{
    SceneNode *sn = SceneGraph::nodeNamed("maincam");
    SceneGraph::present(sn);
    drawDebugLines(sn);
}
