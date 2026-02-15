#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#ifdef __APPLE__
  #include <OpenGL/gl.h>
  #include <OpenGL/glu.h>
  #include <GLUT/glut.h>
#else
  #include <GL/gl.h>
  #include <GL/glu.h>
  #include <GL/glut.h>
#endif


// Datenstruktur zur Darstellung eines 3D-Vektors
typedef struct { 
    double x, y, z; 
} Vector3;


// Erzeugt einen neuen 3D-Vektor
static Vector3 v3(double x, double y, double z) { 
    Vector3 v = {x,y,z}; 
    return v; 
}

// Addition zweier Vektoren
static Vector3 vadd(Vector3 a, Vector3 b) { 
    return v3(a.x+b.x, a.y+b.y, a.z+b.z); 
}

// Subtraktion zweier Vektoren
static Vector3 vsub(Vector3 a, Vector3 b) { 
    return v3(a.x-b.x, a.y-b.y, a.z-b.z); 
}

// Skalarmultiplikation
static Vector3 vmul(Vector3 a, double s) { 
    return v3(a.x*s, a.y*s, a.z*s); 
}

// Skalarprodukt
static double vdot(Vector3 a, Vector3 b) { 
    return a.x*b.x + a.y*b.y + a.z*b.z; 
}

// Länge (Norm) eines Vektors
static double vlen(Vector3 a) { 
    return sqrt(vdot(a,a)); 
}

// Normierung eines Vektors
static Vector3 vnorm(Vector3 a) {
  double L = vlen(a);
  if (L < 1e-12) return v3(0,0,0);
  return vmul(a, 1.0/L);
}


// Datenstruktur für Kugeln in der Szene
typedef struct {
  Vector3 c;        // Mittelpunkt der Kugel
  float r;          // Radius
  float col[3];     // RGB-Farbwerte
} Sphere;


// Maximale Anzahl an Kugeln
#define MAX_SPHERES 2048

static Sphere spheres[MAX_SPHERES];
static int sphereCount = 0;


// Fenstergröße
static int winW = 900, winH = 700;


//Würfelparameter (zentriert)
static const float cubeSize = 1.0f;   // Seitenlänge
static const float cubeHalf = 0.5f;   // halbe Seitenlänge


//Kameraparameter
static Vector3 camPos  = { 2.2, 1.8, 2.6 }; // Kameraposition
static Vector3 camLook = { 0.0, 0.0, 0.0 }; // Blickpunkt
static Vector3 camUp   = { 0.0, 1.0, 0.0 }; // Aufwärtsrichtung


//Zufallsfunktionen zur Erzeugung von Radius und Farbe
// Zufallswert im Bereich [0,1]
static float frand01(void) { 
    return (float)rand() / (float)RAND_MAX; 
}

// Zufallswert im Bereich [a,b]
static float frandRange(float a, float b) { 
    return a + (b-a)*frand01(); 
}

// Erzeugt eine Kugel mit zufälliger Größe und Farbe
static Sphere makeRandomSphere(Vector3 center) {
  Sphere s;
  s.c = center;
  s.r = frandRange(0.06f, 0.16f);   // zufälliger Radius
  s.col[0] = frandRange(0.2f, 1.0f);
  s.col[1] = frandRange(0.2f, 1.0f);
  s.col[2] = frandRange(0.2f, 1.0f);
  return s;
}


//Picking: Umrechnung Fensterkoordinate → Weltkoordinate
static int unprojectAt(int mouseX, int mouseY, 
                       Vector3* outWorld, 
                       float* outDepth) {

  // Umrechnung der Y-Koordinate (GLUT vs. OpenGL Ursprung)
  int glY = winH - 1 - mouseY;

  // Tiefenwert aus dem Z-Buffer auslesen
  float depth = 1.0f;
  glReadPixels(mouseX, glY, 1, 1, 
               GL_DEPTH_COMPONENT, GL_FLOAT, &depth);

  // Falls Hintergrund angeklickt wurde
  if (depth >= 0.999999f) return 0;

  // Aktuelle Transformationsmatrizen abrufen
  GLdouble model[16], proj[16];
  GLint viewport[4];

  glGetDoublev(GL_MODELVIEW_MATRIX, model);
  glGetDoublev(GL_PROJECTION_MATRIX, proj);
  glGetIntegerv(GL_VIEWPORT, viewport);

  GLdouble wx, wy, wz;

  // Rücktransformation mittels gluUnProject
  if (gluUnProject(mouseX, glY, depth,
                   model, proj, viewport,
                   &wx, &wy, &wz) == GL_TRUE) {

    *outWorld = v3(wx, wy, wz);
    if (outDepth) *outDepth = depth;
    return 1;
  }

  return 0;
}


// Überprüfung, ob eine Kugel angeklickt wurde
static int findClickedSphere(Vector3 worldPoint) {

  const double eps = 0.03;  // Toleranzbereich
  int best = -1;
  double bestErr = 1e9;

  for (int i=0; i<sphereCount; i++) {

    Vector3 d = vsub(worldPoint, spheres[i].c);
    double dist = vlen(d);

    // Prüfen, ob Punkt nahe an der Kugeloberfläche liegt
    double err = fabs(dist - spheres[i].r);

    if (err < eps && err < bestErr) {
      bestErr = err;
      best = i;
    }
  }
  return best;
}


// Bestimmung der Würfelfläche
static int cubeFaceNormal(Vector3 p, Vector3* outN) {

  const double eps = 0.02;

  if (fabs(p.x - cubeHalf) < eps) { *outN = v3( 1,0,0); return 1; }
  if (fabs(p.x + cubeHalf) < eps) { *outN = v3(-1,0,0); return 1; }
  if (fabs(p.y - cubeHalf) < eps) { *outN = v3(0, 1,0); return 1; }
  if (fabs(p.y + cubeHalf) < eps) { *outN = v3(0,-1,0); return 1; }
  if (fabs(p.z - cubeHalf) < eps) { *outN = v3(0,0, 1); return 1; }
  if (fabs(p.z + cubeHalf) < eps) { *outN = v3(0,0,-1); return 1; }

  return 0;
}


// Hinzufügen einer Kugel zur Szene
static void addSphere(Sphere s) {
  if (sphereCount >= MAX_SPHERES) return;
  spheres[sphereCount++] = s;
}


// Beleuchtung
static void setupLights(void) {

  glEnable(GL_LIGHTING);
  glEnable(GL_LIGHT0);
  glEnable(GL_COLOR_MATERIAL);

  glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

  GLfloat pos[] = { 3.0f, 4.0f, 5.0f, 1.0f };
  glLightfv(GL_LIGHT0, GL_POSITION, pos);

  GLfloat amb[] = { 0.15f, 0.15f, 0.15f, 1.0f };
  GLfloat dif[] = { 0.90f, 0.90f, 0.90f, 1.0f };

  glLightfv(GL_LIGHT0, GL_AMBIENT, amb);
  glLightfv(GL_LIGHT0, GL_DIFFUSE, dif);
}


//Rendering-Funktion
static void display(void) {

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  // Kameratransformation
  gluLookAt(camPos.x, camPos.y, camPos.z,
            camLook.x, camLook.y, camLook.z,
            camUp.x, camUp.y, camUp.z);

  // Würfel zeichnen
  glColor3f(0.85f, 0.85f, 0.88f);
  glutSolidCube(cubeSize);

  // Kugeln zeichnen
  for (int i=0; i<sphereCount; i++) {

    glPushMatrix();

      glTranslatef((float)spheres[i].c.x,
                   (float)spheres[i].c.y,
                   (float)spheres[i].c.z);

      glColor3fv(spheres[i].col);
      glutSolidSphere(spheres[i].r, 24, 18);

    glPopMatrix();
  }

  glutSwapBuffers();
}


//Fensteranpassung
static void reshape(int w, int h) {

  winW = (w > 1) ? w : 1;
  winH = (h > 1) ? h : 1;

  glViewport(0, 0, winW, winH);

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();

  // Perspektivische Projektion
  gluPerspective(60.0, 
    (double)winW/(double)winH, 
    0.1, 50.0);

  glMatrixMode(GL_MODELVIEW);
}


//Mausinteraktion
static void mouse(int button, int state, int x, int y) {

  if (button != GLUT_LEFT_BUTTON || state != GLUT_DOWN) return;

  glutPostRedisplay();
  glFinish();

  Vector3 worldPoint;
  float depth;

  // Weltkoordinate bestimmen
  if (!unprojectAt(x, y, &worldPoint, &depth))
    return;

  Sphere newS = makeRandomSphere(worldPoint);

  // Prüfen, ob eine Kugel angeklickt wurde
  int hit = findClickedSphere(worldPoint);

  if (hit >= 0) {

    Sphere* base = &spheres[hit];

    Vector3 dir = vsub(worldPoint, base->c);
    dir = vnorm(dir);

    if (vlen(dir) < 1e-12) 
        dir = v3(0,1,0);

    // Neue Kugel so positionieren, dass sie die alte berührt
    Vector3 newC = vadd(base->c, 
      vmul(dir, base->r + newS.r));

    newS.c = newC;
    addSphere(newS);
    glutPostRedisplay();
    return;
  }

  // Falls Würfelfläche angeklickt wurde
  Vector3 n;

  if (cubeFaceNormal(worldPoint, &n)) {
    newS.c = vadd(worldPoint, vmul(n, newS.r));
    addSphere(newS);
    glutPostRedisplay();
    return;
  }

  addSphere(newS);
  glutPostRedisplay();
}


//Tastatursteuerung
static void keyboard(unsigned char key, int x, int y) {

  if (key == 'q' || key == 27) exit(0);
  if (key == 'r') {
    sphereCount = 0;
    glutPostRedisplay();
  }
}


//Initialisierung
static void initGL(void) {

  glEnable(GL_DEPTH_TEST);  // Aktivierung des Z-Tests
  glClearColor(0.08f, 0.09f, 0.11f, 1.0f);
  setupLights();
}


// Hauptprogramm
int main(int argc, char** argv) {

  srand((unsigned)time(NULL));

  glutInit(&argc, argv);
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
  glutInitWindowSize(winW, winH);

  glutCreateWindow("Würfel");

  initGL();

  glutDisplayFunc(display);
  glutReshapeFunc(reshape);
  glutMouseFunc(mouse);
  glutKeyboardFunc(keyboard);

  glutMainLoop();

  return 0;
}
