
#if !defined(rc_scene_h)
#define rc_scene_h

class GameObject;

void loadScene(char const *name);
void stepScene();
extern bool sceneRunning;
void addSceneObject(GameObject *obj);
void assertInScene(GameObject *obj, bool inScene = true);
void deleteSceneObject(GameObject *obj);
void renderScene();

#endif  //  rc_scene_h
