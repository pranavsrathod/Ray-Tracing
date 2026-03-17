/* **************************
 * CSCI 420
 * Assignment 3 Raytracer
 * Name: <Your name here>
 * *************************
*/

#ifdef WIN32
  #include <windows.h>
#endif

#if defined(WIN32) || defined(linux)
  #include <GL/gl.h>
  #include <GL/glut.h>
#elif defined(__APPLE__)
  #include <OpenGL/gl.h>
  #include <GLUT/glut.h>
#endif

#ifdef _OPENMP
  #include <omp.h>
#endif

#include <functional>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef WIN32
  #define strcasecmp _stricmp
#endif

#include <imageIO.h>
#include <cmath>

#define MAX_TRIANGLES 20000
#define MAX_SPHERES 100
#define MAX_LIGHTS 100

char * filename = NULL;

// The different display modes.
#define MODE_DISPLAY 1
#define MODE_JPEG 2

int mode = MODE_DISPLAY;

// While solving the homework, it is useful to make the below values smaller for debugging purposes.
// The still images that you need to submit with the homework should be at the below resolution (640x480).
// However, for your own purposes, after you have solved the homework, you can increase those values to obtain higher-resolution images.
#define WIDTH 640
#define HEIGHT 480

// The field of view of the camera, in degrees.
#define fov 60.0

// Buffer to store the image when saving it to a JPEG.
unsigned char buffer[HEIGHT][WIDTH][3];

struct Vertex
{
  double position[3];
  double color_diffuse[3];
  double color_specular[3];
  double normal[3];
  double shininess;
};

struct Triangle
{
  Vertex v[3];
};

struct Sphere
{
  double position[3];
  double color_diffuse[3];
  double color_specular[3];
  double shininess;
  double radius;
};

struct Light
{
  double position[3];
  double color[3];
};

Triangle triangles[MAX_TRIANGLES];
Sphere spheres[MAX_SPHERES];
Light lights[MAX_LIGHTS];
double ambient_light[3];

int num_triangles = 0;
int num_spheres = 0;
int num_lights = 0;

void plot_pixel_display(int x,int y,unsigned char r,unsigned char g,unsigned char b);
void plot_pixel_jpeg(int x,int y,unsigned char r,unsigned char g,unsigned char b);
void plot_pixel(int x,int y,unsigned char r,unsigned char g,unsigned char b);

// Forward declarations
bool intersectSphere(double origin[3], double dir[3], Sphere& s, double& t);
bool intersectTriangle(double origin[3], double dir[3], Triangle& tri, double& t, double& alpha, double& beta, double& gamma);


// Helper: dot product
double dot(double a[3], double b[3])
{
  return a[0]*b[0] + a[1]*b[1] + a[2]*b[2];
}

// Helper: normalize in place
void normalize(double v[3])
{
  double len = sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
  v[0] /= len; v[1] /= len; v[2] /= len;
}

// ─── PATH TRACER HELPERS ──────────────────────────────────────────────────

#include <random>

// Random double in [0, 1)
static std::mt19937 rng(42);
static std::uniform_real_distribution<double> dist(0.0, 1.0);
inline double randomDouble() { return dist(rng); }

// Cross product: out = a x b
void cross(double a[3], double b[3], double out[3])
{
  out[0] = a[1]*b[2] - a[2]*b[1];
  out[1] = a[2]*b[0] - a[0]*b[2];
  out[2] = a[0]*b[1] - a[1]*b[0];
}

// Build an orthonormal basis (tangent, bitangent) around a normal N.
// We need this to transform our sampled hemisphere direction into world space.
void buildFrame(double N[3], double T[3], double B[3])
{
  // Pick an arbitrary vector not parallel to N
  double up[3] = {0.0, 1.0, 0.0};
  if(fabs(N[1]) > 0.99) { up[0]=1.0; up[1]=0.0; up[2]=0.0; }

  cross(up, N, T);
  normalize(T);
  cross(N, T, B); // B is already unit length since N and T are orthonormal
}

// Cosine-weighted hemisphere sample in world space around normal N.
// r1, r2 are uniform random numbers in [0,1).
// Why cosine-weighted? It importance-samples the cosine term in the
// rendering equation, so the pdf cancels cleanly: (cosθ/π) / (cosθ/π) = 1
void sampleHemisphere(double N[3], double out[3])
{
  double r1 = randomDouble();
  double r2 = randomDouble();

  // Spherical coords in local space
  // cosθ = sqrt(r1) gives cosine-weighted distribution
  double sinTheta = sqrt(1.0 - r1);  // sin(arccos(sqrt(r1)))
  double cosTheta = sqrt(r1);
  double phi = 2.0 * M_PI * r2;

  // Local direction (x,y,z) where y is "up" (along N)
  double localX = sinTheta * cos(phi);
  double localY = cosTheta;            // component along N
  double localZ = sinTheta * sin(phi);

  // Build frame around N and transform to world space
  double T[3], B[3];
  buildFrame(N, T, B);

  out[0] = localX*T[0] + localY*N[0] + localZ*B[0];
  out[1] = localX*T[1] + localY*N[1] + localZ*B[1];
  out[2] = localX*T[2] + localY*N[2] + localZ*B[2];
}

// ─────────────────────────────────────────────────────────────────────────────

bool inShadow(double hit[3], double lightPos[3])
{
  double L[3] = {
    lightPos[0] - hit[0],
    lightPos[1] - hit[1],
    lightPos[2] - hit[2]
  };
  double distToLight = sqrt(L[0]*L[0] + L[1]*L[1] + L[2]*L[2]);
  normalize(L);

  for(int i = 0; i < num_spheres; i++)
  {
    double t;
    if(intersectSphere(hit, L, spheres[i], t))
      if(t > 1e-4 && t < distToLight) return true;  // ← added t > 1e-4
  }
  for(int i = 0; i < num_triangles; i++)
  {
    double t, alpha, beta, gamma;
    if(intersectTriangle(hit, L, triangles[i], t, alpha, beta, gamma))
      if(t > 1e-4 && t < distToLight) return true;  // ← added t > 1e-4
  }
  return false;
}

void spherePhong(double hit[3], double dir[3], Sphere& s, double color[3])
{
  // Normal at hit point (pointing outward from sphere center)
  double N[3] = {
    (hit[0] - s.position[0]) / s.radius,
    (hit[1] - s.position[1]) / s.radius,
    (hit[2] - s.position[2]) / s.radius
  };

  // View direction (from hit point back to camera)
  double V[3] = {-dir[0], -dir[1], -dir[2]};
  normalize(V);

  // Start with ambient
  color[0] = ambient_light[0] * s.color_diffuse[0];
  color[1] = ambient_light[1] * s.color_diffuse[1];
  color[2] = ambient_light[2] * s.color_diffuse[2];

  // Sum contributions from each light
  for(int i = 0; i < num_lights; i++)
  {
    if(inShadow(hit, lights[i].position)) continue;
    // L = direction from hit point to light
    double L[3] = {
      lights[i].position[0] - hit[0],
      lights[i].position[1] - hit[1],
      lights[i].position[2] - hit[2]
    };
    normalize(L);

    // R = reflection of L about N: R = 2(L·N)N - L
    double LdotN = fmax(0.0, dot(L, N));
    double R[3] = {
      2.0*LdotN*N[0] - L[0],
      2.0*LdotN*N[1] - L[1],
      2.0*LdotN*N[2] - L[2]
    };
    normalize(R);

    double RdotV = fmax(0.0, dot(R, V));

    // Phong equation per channel
    for(int c = 0; c < 3; c++)
    {
      color[c] += lights[i].color[c] * (
        s.color_diffuse[c]  * LdotN +
        s.color_specular[c] * pow(RdotV, s.shininess)
      );
    }
  }

  // Clamp to [0, 1]
  for(int c = 0; c < 3; c++)
    color[c] = fmin(1.0, color[c]);
}
bool intersectSphere(double origin[3], double dir[3], Sphere& s, double& t)
{
  double oc[3] = {
    origin[0] - s.position[0],
    origin[1] - s.position[1],
    origin[2] - s.position[2]
  };

  double a = dir[0]*dir[0] + dir[1]*dir[1] + dir[2]*dir[2];
  double b = 2.0 * (dir[0]*oc[0] + dir[1]*oc[1] + dir[2]*oc[2]);
  double c = oc[0]*oc[0] + oc[1]*oc[1] + oc[2]*oc[2] - s.radius * s.radius;

  double disc = b*b - 4*a*c;
  if(disc < 0) return false;

  double sqrtDisc = sqrt(disc);
  double t1 = (-b - sqrtDisc) / (2.0 * a);
  double t2 = (-b + sqrtDisc) / (2.0 * a);

  // Take smallest positive t
  if(t1 > 1e-4) { t = t1; return true; }
  if(t2 > 1e-4) { t = t2; return true; }
  return false;
}

void trianglePhong(double hit[3], double dir[3], Triangle& tri, double alpha, double beta, double gamma, double color[3])
{
  // Interpolate normal using barycentric coords
  double N[3];
  for(int c = 0; c < 3; c++)
    N[c] = alpha * tri.v[0].normal[c]
         + beta  * tri.v[1].normal[c]
         + gamma * tri.v[2].normal[c];
  normalize(N);

  // Interpolate diffuse, specular, shininess
  double kd[3], ks[3], sh = 0;
  for(int c = 0; c < 3; c++)
  {
    kd[c] = alpha * tri.v[0].color_diffuse[c]
           + beta  * tri.v[1].color_diffuse[c]
           + gamma * tri.v[2].color_diffuse[c];
    ks[c] = alpha * tri.v[0].color_specular[c]
           + beta  * tri.v[1].color_specular[c]
           + gamma * tri.v[2].color_specular[c];
  }
  sh = alpha * tri.v[0].shininess
     + beta  * tri.v[1].shininess
     + gamma * tri.v[2].shininess;

  // View direction
  double V[3] = {-dir[0], -dir[1], -dir[2]};
  normalize(V);

  // Ambient
  color[0] = ambient_light[0] * kd[0];
  color[1] = ambient_light[1] * kd[1];
  color[2] = ambient_light[2] * kd[2];

  // Sum contributions from each light
  for(int i = 0; i < num_lights; i++)
  {
    if(inShadow(hit, lights[i].position)) continue;

    double L[3] = {
      lights[i].position[0] - hit[0],
      lights[i].position[1] - hit[1],
      lights[i].position[2] - hit[2]
    };
    normalize(L);

    double LdotN = fmax(0.0, dot(L, N));
    double R[3] = {
      2.0*LdotN*N[0] - L[0],
      2.0*LdotN*N[1] - L[1],
      2.0*LdotN*N[2] - L[2]
    };
    normalize(R);

    double RdotV = fmax(0.0, dot(R, V));

    for(int c = 0; c < 3; c++)
    {
      color[c] += lights[i].color[c] * (
        kd[c] * LdotN +
        ks[c] * pow(RdotV, sh)
      );
    }
  }

  // Clamp
  for(int c = 0; c < 3; c++)
    color[c] = fmin(1.0, color[c]);
}

bool intersectTriangle(double origin[3], double dir[3], Triangle& tri, double& t, double& alpha, double& beta, double& gamma)
{
  double* A = tri.v[0].position;
  double* B = tri.v[1].position;
  double* C = tri.v[2].position;

  // Edge vectors
  double AB[3] = {B[0]-A[0], B[1]-A[1], B[2]-A[2]};
  double AC[3] = {C[0]-A[0], C[1]-A[1], C[2]-A[2]};

  // Face normal
  double N[3] = {
    AB[1]*AC[2] - AB[2]*AC[1],
    AB[2]*AC[0] - AB[0]*AC[2],
    AB[0]*AC[1] - AB[1]*AC[0]
  };

  double denom = N[0]*dir[0] + N[1]*dir[1] + N[2]*dir[2];
  if(fabs(denom) < 1e-8) return false; // Ray parallel to triangle

  // Distance to plane
  double d = N[0]*A[0] + N[1]*A[1] + N[2]*A[2];
  t = (d - (N[0]*origin[0] + N[1]*origin[1] + N[2]*origin[2])) / denom;
  if(t < 1e-4) return false;

  // Hit point
  double P[3] = {
    origin[0] + t*dir[0],
    origin[1] + t*dir[1],
    origin[2] + t*dir[2]
  };

  // Barycentric coordinates using area method
  double areaABC = N[0]*N[0] + N[1]*N[1] + N[2]*N[2]; // proportional to area

  // Alpha (weight for vertex A) — opposite sub-triangle BCP
  double BC[3] = {C[0]-B[0], C[1]-B[1], C[2]-B[2]};
  double BP[3] = {P[0]-B[0], P[1]-B[1], P[2]-B[2]};
  double cross1[3] = {BC[1]*BP[2]-BC[2]*BP[1], BC[2]*BP[0]-BC[0]*BP[2], BC[0]*BP[1]-BC[1]*BP[0]};
  alpha = (N[0]*cross1[0] + N[1]*cross1[1] + N[2]*cross1[2]) / areaABC;

  // Beta (weight for vertex B) — opposite sub-triangle CAP
  double CA[3] = {A[0]-C[0], A[1]-C[1], A[2]-C[2]};
  double CP[3] = {P[0]-C[0], P[1]-C[1], P[2]-C[2]};
  double cross2[3] = {CA[1]*CP[2]-CA[2]*CP[1], CA[2]*CP[0]-CA[0]*CP[2], CA[0]*CP[1]-CA[1]*CP[0]};
  beta = (N[0]*cross2[0] + N[1]*cross2[1] + N[2]*cross2[2]) / areaABC;

  gamma = 1.0 - alpha - beta;

  // Check if point is inside triangle
  if(alpha < 0 || beta < 0 || gamma < 0) return false;

  return true;
}

// ─── RECURSIVE PATH TRACE ────────────────────────────────────────────────

#define MAX_DEPTH 5
#define SAMPLES 512   // increase to 256+ for final renders

// Returns the radiance along a ray (origin, dir).
// depth counts how many bounces we've done so far.
// Change signature to accept rng as a parameter
void traceRay(double origin[3], double dir[3], int depth, double color[3],
  std::function<double()>& randFunc)
{
  color[0] = color[1] = color[2] = 0.0;

  // ── 1. Find closest intersection ────────────────────────────────────────
  double closest_t = 1e18;
  int hit_sphere   = -1;
  int hit_triangle = -1;
  double hit_alpha, hit_beta, hit_gamma;

  for(int i = 0; i < num_spheres; i++)
  {
    double t;
    if(intersectSphere(origin, dir, spheres[i], t) && t < closest_t)
    {
      closest_t  = t;
      hit_sphere = i;
      hit_triangle = -1;
    }
  }
  for(int i = 0; i < num_triangles; i++)
  {
    double t, a, b, g;
    if(intersectTriangle(origin, dir, triangles[i], t, a, b, g) && t < closest_t)
    {
      closest_t    = t;
      hit_triangle = i;
      hit_sphere   = -1;
      hit_alpha = a; hit_beta = b; hit_gamma = g;
    }
  }

  // ── 2. No hit → black (or you can return a sky color) ───────────────────
  if(hit_sphere < 0 && hit_triangle < 0)
  {
    color[0] = color[1] = color[2] = 0.0; // black background
    return;
  }

  // ── 3. Compute hit point, normal, and albedo ─────────────────────────────
  double hit[3] = {
    origin[0] + closest_t*dir[0],
    origin[1] + closest_t*dir[1],
    origin[2] + closest_t*dir[2]
  };

  double N[3];       // surface normal at hit
  double albedo[3];  // diffuse color at hit

  if(hit_sphere >= 0)
  {
    Sphere& s = spheres[hit_sphere];
    N[0] = (hit[0]-s.position[0])/s.radius;
    N[1] = (hit[1]-s.position[1])/s.radius;
    N[2] = (hit[2]-s.position[2])/s.radius;
    albedo[0] = s.color_diffuse[0];
    albedo[1] = s.color_diffuse[1];
    albedo[2] = s.color_diffuse[2];
  }
  else
  {
    Triangle& tri = triangles[hit_triangle];
    for(int c = 0; c < 3; c++)
    {
      N[c] = hit_alpha*tri.v[0].normal[c]
           + hit_beta *tri.v[1].normal[c]
           + hit_gamma*tri.v[2].normal[c];
      albedo[c] = hit_alpha*tri.v[0].color_diffuse[c]
                + hit_beta *tri.v[1].color_diffuse[c]
                + hit_gamma*tri.v[2].color_diffuse[c];
    }
    normalize(N);
  }

  // Flip normal if it's facing away from the incoming ray
  // (handles back-face hits, e.g. inside a box)
  if(dot(N, dir) > 0)
  { N[0]=-N[0]; N[1]=-N[1]; N[2]=-N[2]; }

  // ── 4. Emission — treat point lights as a bonus direct term ──────────────
  // We keep your existing point lights as direct light sampling (Next Event
  // Estimation lite) so the scene files you already have still look good.
  double direct[3] = {0,0,0};
  for(int i = 0; i < num_lights; i++)
  {
    if(inShadow(hit, lights[i].position)) continue;

    double L[3] = {
      lights[i].position[0]-hit[0],
      lights[i].position[1]-hit[1],
      lights[i].position[2]-hit[2]
    };
    normalize(L);
    double LdotN = fmax(0.0, dot(L, N));
    for(int c = 0; c < 3; c++)
      direct[c] += lights[i].color[c] * albedo[c] * LdotN;
  }

  // ── 5. Russian Roulette termination ──────────────────────────────────────
  // Survival probability based on albedo brightness
  double survive = fmax(albedo[0], fmax(albedo[1], albedo[2]));
  survive = fmax(0.1, fmin(survive, 0.95)); // clamp so we always have a chance

  if(depth >= MAX_DEPTH || randFunc() > survive)
  {
    // Terminate — return only direct lighting
    for(int c = 0; c < 3; c++)
      color[c] = fmin(1.0, direct[c]);
    return;
  }

  // ── 6. Sample a random bounce direction ──────────────────────────────────
  double bounceDir[3];
  {
    double r1 = randFunc();
    double r2 = randFunc();
    double sinTheta = sqrt(1.0 - r1);
    double cosTheta = sqrt(r1);
    double phi = 2.0 * M_PI * r2;
    double T[3], B[3];
    buildFrame(N, T, B);
    bounceDir[0] = sinTheta*cos(phi)*T[0] + cosTheta*N[0] + sinTheta*sin(phi)*B[0];
    bounceDir[1] = sinTheta*cos(phi)*T[1] + cosTheta*N[1] + sinTheta*sin(phi)*B[1];
    bounceDir[2] = sinTheta*cos(phi)*T[2] + cosTheta*N[2] + sinTheta*sin(phi)*B[2];
  }

  // Offset origin slightly along normal to avoid self-intersection
  double bounceOrigin[3] = {
    hit[0] + N[0]*1e-4,
    hit[1] + N[1]*1e-4,
    hit[2] + N[2]*1e-4
  };

  // ── 7. Recurse ────────────────────────────────────────────────────────────
  double indirect[3];
  traceRay(bounceOrigin, bounceDir, depth+1, indirect, randFunc);

  // ── 8. Combine: direct + indirect * albedo / survive ─────────────────────
  // The albedo/π * π from cosine-weighted pdf cancel, leaving just albedo.
  // We divide by survive to compensate for Russian Roulette termination.
  for(int c = 0; c < 3; c++)
    color[c] = fmin(1.0, direct[c] + (albedo[c] * indirect[c]) / survive);
}

// ─────────────────────────────────────────────────────────────────────────────

void draw_scene()
{
  double aspect     = (double)WIDTH / (double)HEIGHT;
  double halfHeight = tan((fov / 2.0) * M_PI / 180.0);
  double halfWidth  = aspect * halfHeight;

  for(unsigned int x = 0; x < WIDTH; x++)
  {
    std::mt19937 localRng(42 + x);
    std::uniform_real_distribution<double> localDist(0.0, 1.0);
    std::function<double()> randLocal = [&]() { return localDist(localRng); };

    for(unsigned int y = 0; y < HEIGHT; y++)
    {
      double accum[3] = {0.0, 0.0, 0.0};

      for(int s = 0; s < SAMPLES; s++)
      {
        double jx = randLocal();
        double jy = randLocal();

        double nx = (2.0 * (x + jx) / WIDTH  - 1.0) * halfWidth;
        double ny = (2.0 * (y + jy) / HEIGHT - 1.0) * halfHeight;

        double origin[3]    = {0.0, 0.0, 0.0};
        double direction[3] = {nx, ny, -1.0};
        normalize(direction);

        double color[3];
        traceRay(origin, direction, 0, color, randLocal);

        accum[0] += color[0];
        accum[1] += color[1];
        accum[2] += color[2];
      }

      unsigned char r = (unsigned char)(fmin(1.0, accum[0]/SAMPLES) * 255);
      unsigned char g = (unsigned char)(fmin(1.0, accum[1]/SAMPLES) * 255);
      unsigned char b = (unsigned char)(fmin(1.0, accum[2]/SAMPLES) * 255);

      plot_pixel(x, y, r, g, b);
    }

    printf("Progress: %d / %d\n", x+1, WIDTH);
    fflush(stdout);
  }

  // Draw all pixels to screen after rendering is done
  glPointSize(2.0);
  glBegin(GL_POINTS);
  for(unsigned int x = 0; x < WIDTH; x++)
    for(unsigned int y = 0; y < HEIGHT; y++)
    {
      glColor3f(buffer[y][x][0]/255.0f, buffer[y][x][1]/255.0f, buffer[y][x][2]/255.0f);
      glVertex2i(x, y);
    }
  glEnd();
  glFlush();

  printf("Path tracing completed.\n");
  fflush(stdout);
}


void plot_pixel_display(int x, int y, unsigned char r, unsigned char g, unsigned char b)
{
  // glColor3f(((float)r) / 255.0f, ((float)g) / 255.0f, ((float)b) / 255.0f);
  // glVertex2i(x,y);
}

void plot_pixel_jpeg(int x, int y, unsigned char r, unsigned char g, unsigned char b)
{
  buffer[y][x][0] = r;
  buffer[y][x][1] = g;
  buffer[y][x][2] = b;
}

// void plot_pixel(int x, int y, unsigned char r, unsigned char g, unsigned char b)
// {
//   plot_pixel_display(x,y,r,g,b);
//   if(mode == MODE_JPEG)
//     plot_pixel_jpeg(x,y,r,g,b);
// }

void plot_pixel(int x, int y, unsigned char r, unsigned char g, unsigned char b)
{
  plot_pixel_display(x,y,r,g,b);
  plot_pixel_jpeg(x,y,r,g,b); // always write to buffer
}

void save_jpg()
{
  printf("Saving JPEG file: %s\n", filename);

  ImageIO img(WIDTH, HEIGHT, 3, &buffer[0][0][0]);
  if (img.save(filename, ImageIO::FORMAT_JPEG) != ImageIO::OK)
    printf("Error in saving\n");
  else 
    printf("File saved successfully\n");
}

void parse_check(const char *expected, char *found)
{
  if(strcasecmp(expected,found))
  {
    printf("Expected '%s ' found '%s '\n", expected, found);
    printf("Parsing error; abnormal program abortion.\n");
    exit(0);
  }
}

void parse_doubles(FILE* file, const char *check, double p[3])
{
  char str[100];
  fscanf(file,"%s",str);
  parse_check(check,str);
  fscanf(file,"%lf %lf %lf",&p[0],&p[1],&p[2]);
  printf("%s %lf %lf %lf\n",check,p[0],p[1],p[2]);
}

void parse_rad(FILE *file, double *r)
{
  char str[100];
  fscanf(file,"%s",str);
  parse_check("rad:",str);
  fscanf(file,"%lf",r);
  printf("rad: %f\n",*r);
}

void parse_shi(FILE *file, double *shi)
{
  char s[100];
  fscanf(file,"%s",s);
  parse_check("shi:",s);
  fscanf(file,"%lf",shi);
  printf("shi: %f\n",*shi);
}

int loadScene(char *argv)
{
  FILE * file = fopen(argv,"r");
  if (!file)
  {
    printf("Unable to open input file %s. Program exiting.\n", argv);
    exit(0);
  }

  int number_of_objects;
  char type[50];
  Triangle t;
  Sphere s;
  Light l;
  fscanf(file,"%i", &number_of_objects);

  printf("number of objects: %i\n",number_of_objects);

  parse_doubles(file,"amb:",ambient_light);

  for(int i=0; i<number_of_objects; i++)
  {
    fscanf(file,"%s\n",type);
    printf("%s\n",type);
    if(strcasecmp(type,"triangle")==0)
    {
      printf("found triangle\n");
      for(int j=0;j < 3;j++)
      {
        parse_doubles(file,"pos:",t.v[j].position);
        parse_doubles(file,"nor:",t.v[j].normal);
        parse_doubles(file,"dif:",t.v[j].color_diffuse);
        parse_doubles(file,"spe:",t.v[j].color_specular);
        parse_shi(file,&t.v[j].shininess);
      }

      if(num_triangles == MAX_TRIANGLES)
      {
        printf("too many triangles, you should increase MAX_TRIANGLES!\n");
        exit(0);
      }
      triangles[num_triangles++] = t;
    }
    else if(strcasecmp(type,"sphere")==0)
    {
      printf("found sphere\n");

      parse_doubles(file,"pos:",s.position);
      parse_rad(file,&s.radius);
      parse_doubles(file,"dif:",s.color_diffuse);
      parse_doubles(file,"spe:",s.color_specular);
      parse_shi(file,&s.shininess);

      if(num_spheres == MAX_SPHERES)
      {
        printf("too many spheres, you should increase MAX_SPHERES!\n");
        exit(0);
      }
      spheres[num_spheres++] = s;
    }
    else if(strcasecmp(type,"light")==0)
    {
      printf("found light\n");
      parse_doubles(file,"pos:",l.position);
      parse_doubles(file,"col:",l.color);

      if(num_lights == MAX_LIGHTS)
      {
        printf("too many lights, you should increase MAX_LIGHTS!\n");
        exit(0);
      }
      lights[num_lights++] = l;
    }
    else
    {
      printf("unknown type in scene description:\n%s\n",type);
      exit(0);
    }
  }
  return 0;
}

void display()
{
}

void init()
{
  glMatrixMode(GL_PROJECTION);
  glOrtho(0,WIDTH,0,HEIGHT,1,-1);
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  glClearColor(0,0,0,0);
  glClear(GL_COLOR_BUFFER_BIT);
}

void idle()
{
  // Hack to make it only draw once.
  static int once=0;
  if(!once)
  {
    draw_scene();
    if(mode == MODE_JPEG)
      save_jpg();
      exit(0);
  }
  once=1;
}

int main(int argc, char ** argv)
{
  if ((argc < 2) || (argc > 3))
  {  
    printf ("Usage: %s <input scenefile> [output jpegname]\n", argv[0]);
    exit(0);
  }
  if(argc == 3)
  {
    mode = MODE_JPEG;
    filename = argv[2];
  }
  else if(argc == 2)
    mode = MODE_DISPLAY;

  glutInit(&argc,argv);
  loadScene(argv[1]);

  glutInitDisplayMode(GLUT_RGBA | GLUT_SINGLE);
  glutInitWindowPosition(0,0);
  glutInitWindowSize(WIDTH,HEIGHT);
  int window = glutCreateWindow("Ray Tracer");
  #ifdef __APPLE__
    // This is needed on recent Mac OS X versions to correctly display the window.
    glutReshapeWindow(WIDTH - 1, HEIGHT - 1);
  #endif
  glutDisplayFunc(display);
  glutIdleFunc(idle);
  init();
  glutMainLoop();
}

