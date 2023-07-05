#include <elm/Elm.h>
#include <elm/Map.h>
#include <elm/Timer.h>
#include <elm/path/Pathfinder.h>
//
#include <GLFW/glfw3.h>

using namespace elm;

#include <chrono>

using ms_float = std::chrono::duration<float, std::milli>;

const char* kMapFilename = "jun2018.lvl";

struct PathRequest {
  Vector2f start;
  Vector2f end;
  Vector2f center;

  PathRequest(Vector2f start, Vector2f end, Vector2f center) : start(start), end(end), center(center) {}
};

// PathRequest path_request(Vector2f(512, 512), Vector2f(397, 542), Vector2f(512, 512));  // Center
PathRequest path_request(Vector2f(828, 341), Vector2f(820, 210), Vector2f(830, 265));  // Base 1
// PathRequest path_request(Vector2f(265, 565), Vector2f(155, 585), Vector2f(155, 585));  // Base 6

const bool kEnableLinearWeights = true;

const float kShipRadius = 14.0f / 16.0f;
//float kShipRadius = 39.0f / 16.0f;

int main(int argc, char* argv[]) {
  Timer perf_timer;

  GLFWwindow* window;

  if (!glfwInit()) {
    fprintf(stderr, "Failed to initialize GLFW.\n");
    return 1;
  }

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_RESIZABLE, false);

  int width = 1360;
  int height = 768;

  window = glfwCreateWindow(width, height, "elm", NULL, NULL);
  if (!window) {
    glfwTerminate();
    return 1;
  }

  glfwMakeContextCurrent(window);

  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    fprintf(stderr, "Failed to initialize opengl context.\n");
    return 0;
  }

  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LEQUAL);

  glViewport(0, 0, width, height);

  auto map = Map::Load(kMapFilename);
  if (!map) {
    fprintf(stderr, "Failed to load map '%s'\n", kMapFilename);
    return 1;
  }

  Elm elm(window, Vector2f((float)width, (float)height));

  if (!elm.Initialize(kMapFilename, *map)) {
    fprintf(stderr, "Failed to initialize elm.\n");
    return 1;
  }

  printf("Elm::Init::Time: %lluus\n", perf_timer.GetElapsedTime());

  std::unique_ptr<RegionRegistry> registry = std::make_unique<RegionRegistry>(*map);
  registry->CreateAll(*map, kShipRadius);

  printf("Registry::Time: %lluus\n", perf_timer.GetElapsedTime());

  float frame_time = 0.0f;

  auto processor = std::make_unique<elm::path::NodeProcessor>(*map);
  elm::path::Pathfinder pathfinder(std::move(processor));

  pathfinder.CreateMapWeights(*map, kShipRadius, kEnableLinearWeights);

  printf("Pathfinder::Init::Time: %lluus\n", perf_timer.GetElapsedTime());

#if 0  // A lot of pathing for Performance Profile.
  constexpr size_t kPathCount = 1000;

  std::vector<u64> timers(kPathCount);

  Timer path_timer;
  for (size_t i = 0; i < kPathCount; ++i) {
    auto path = pathfinder.FindPath(path_request.start, path_request.end, kShipRadius);

    printf("Path size: %zd\n", path.size());
    timers[i] = path_timer.GetElapsedTime();
  }

  u64 sum = 0;
  for (size_t i = 0; i < kPathCount; ++i) {
    sum += timers[i];
  }
  u64 avg = sum / kPathCount;
  printf("Path avg time: %llu\n", avg);
#endif

  auto path = pathfinder.FindPath(path_request.start, path_request.end, kShipRadius);

  printf("Pathfinder::FindPath::Time: %lluus\n", perf_timer.GetElapsedTime());

  elm.camera.position = path_request.center;

  if (!path.empty()) {
    for (size_t i = 0; i < path.size() - 1; ++i) {
      elm.line_renderer.PushLine(path[i], Vector3f(1.0f, 0.0f, 0.0f), path[i + 1], Vector3f(1.0f, 0.0f, 0.0f));
    }
  }

  while (!glfwWindowShouldClose(window)) {
    auto start = std::chrono::high_resolution_clock::now();
    float dt = frame_time / 1000.0f;

    glfwPollEvents();

    OGLErrorCheck();

    constexpr float kCameraMoveSpeed = 100.0f;
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
      elm.camera.position.y -= kCameraMoveSpeed * dt;
    }
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
      elm.camera.position.y += kCameraMoveSpeed * dt;
    }
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
      elm.camera.position.x -= kCameraMoveSpeed * dt;
    }
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
      elm.camera.position.x += kCameraMoveSpeed * dt;
    }

    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
      elm.camera.position = path_request.center;
      elm.camera.SetScale(1.0f / 16.0f);
    }

    if (glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS) {
      elm.camera.SetScale(1.0f / 16.0f);
    }

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    elm.Render(false);

    glfwSwapBuffers(window);

    auto end = std::chrono::high_resolution_clock::now();
    frame_time = std::chrono::duration_cast<ms_float>(end - start).count();
  }

  glfwTerminate();

  return 0;
}
