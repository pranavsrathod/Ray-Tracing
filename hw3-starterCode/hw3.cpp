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

void draw_scene()
{
  double aspect = (double)WIDTH / (double)HEIGHT;
  double halfHeight = tan((fov / 2.0) * M_PI / 180.0);
  double halfWidth = aspect * halfHeight;
  
  for(unsigned int x=0; x<WIDTH; x++)
  {
    glPointSize(2.0);  
    // Do not worry about this usage of OpenGL. This is here just so that we can draw the pixels to the screen,
    // after their R,G,B colors were determined by the ray tracer.
    glBegin(GL_POINTS);
    for(unsigned int y=0; y<HEIGHT; y++)
    {

      // Convert pixel coords to camera space
      double nx = (2.0 * (x + 0.5) / WIDTH  - 1.0) * halfWidth;
      double ny = (2.0 * (y + 0.5) / HEIGHT - 1.0) * halfHeight;

      // Ray origin and direction
      double origin[3]    = {0.0, 0.0, 0.0};
      double direction[3] = {nx, ny, -1.0};

      // Normalize direction
      double len = sqrt(direction[0]*direction[0] + direction[1]*direction[1] + direction[2]*direction[2]);
      direction[0] /= len;
      direction[1] /= len;
      direction[2] /= len;

      // Find closest intersection
      double closest_t = 1e18;
      int hit_sphere = -1;

      for(int i = 0; i < num_spheres; i++)
      {
        double t;
        if(intersectSphere(origin, direction, spheres[i], t))
        {
          if(t < closest_t)
          {
            closest_t = t;
            hit_sphere = i;
          }
        }
      }

      int hit_triangle = -1;
      double hit_alpha, hit_beta, hit_gamma;

      for(int i = 0; i < num_triangles; i++)
      {
        double t, alpha, beta, gamma;
        if(intersectTriangle(origin, direction, triangles[i], t, alpha, beta, gamma))
        {
          if(t < closest_t)
          {
            closest_t = t;
            hit_sphere = -1;
            hit_triangle = i;
            hit_alpha = alpha;
            hit_beta = beta;
            hit_gamma = gamma;
          }
        }
      }

      // Color based on hit
      unsigned char r, g, b;
      if(hit_sphere >= 0)
      {
        // Compute hit point
        double hit[3] = {
          origin[0] + closest_t * direction[0],
          origin[1] + closest_t * direction[1],
          origin[2] + closest_t * direction[2]
        };

        double color[3];
        spherePhong(hit, direction, spheres[hit_sphere], color);

        r = (unsigned char)(color[0] * 255);
        g = (unsigned char)(color[1] * 255);
        b = (unsigned char)(color[2] * 255);
      }
      else if(hit_triangle >= 0)
      {
        // Interpolate diffuse color using barycentric coords
        Triangle& tri = triangles[hit_triangle];
        double dc[3];
        for(int c = 0; c < 3; c++)
          dc[c] = hit_alpha * tri.v[0].color_diffuse[c]
                + hit_beta  * tri.v[1].color_diffuse[c]
                + hit_gamma * tri.v[2].color_diffuse[c];
        r = (unsigned char)(dc[0] * 255);
        g = (unsigned char)(dc[1] * 255);
        b = (unsigned char)(dc[2] * 255);
      }
      else
      {
        r = 255; g = 255; b = 255; // white background
      }
      plot_pixel(x, y, r, g, b);

      // // A simple R,G,B output for testing purposes.
      // // Modify these R,G,B colors to the values computed by your ray tracer.
      // unsigned char r = x % 256; // modify
      // unsigned char g = y % 256; // modify
      // unsigned char b = (x+y) % 256; // modify
      // plot_pixel(x, y, r, g, b);
    }
    glEnd();
    glFlush();
  }
  printf("Ray tracing completed.\n"); 
  fflush(stdout);
}

void plot_pixel_display(int x, int y, unsigned char r, unsigned char g, unsigned char b)
{
  glColor3f(((float)r) / 255.0f, ((float)g) / 255.0f, ((float)b) / 255.0f);
  glVertex2i(x,y);
}

void plot_pixel_jpeg(int x, int y, unsigned char r, unsigned char g, unsigned char b)
{
  buffer[y][x][0] = r;
  buffer[y][x][1] = g;
  buffer[y][x][2] = b;
}

void plot_pixel(int x, int y, unsigned char r, unsigned char g, unsigned char b)
{
  plot_pixel_display(x,y,r,g,b);
  if(mode == MODE_JPEG)
    plot_pixel_jpeg(x,y,r,g,b);
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

