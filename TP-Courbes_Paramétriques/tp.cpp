// -------------------------------------------
// gMini : a minimal OpenGL/GLUT application
// for 3D graphics.
// Copyright (C) 2006-2008 Tamy Boubekeur
// All rights reserved.
// -------------------------------------------

// -------------------------------------------
// Disclaimer: this code is dirty in the
// meaning that there is no attention paid to
// proper class attribute access, memory
// management or optimisation of any kind. It
// is designed for quick-and-dirty testing
// purpose.
// -------------------------------------------

#include <GL/gl.h>
#include <iostream>
#include <fstream>
#include <math.h>
#include <vector>
#include <algorithm>
#include <string>
#include <cstdio>
#include <cstdlib>

#include <algorithm>
#include <GL/glut.h>
#include <float.h>
#include "src/Vec3.h"
#include "src/Camera.h"
#include "src/FastNoiseLit.h"


struct Triangle {
    inline Triangle () {
        v[0] = v[1] = v[2] = 0;
    }
    inline Triangle (const Triangle & t) {
        v[0] = t.v[0];   v[1] = t.v[1];   v[2] = t.v[2];
    }
    inline Triangle (unsigned int v0, unsigned int v1, unsigned int v2) {
        v[0] = v0;   v[1] = v1;   v[2] = v2;
    }
    unsigned int & operator [] (unsigned int iv) { return v[iv]; }
    unsigned int operator [] (unsigned int iv) const { return v[iv]; }
    inline virtual ~Triangle () {}
    inline Triangle & operator = (const Triangle & t) {
        v[0] = t.v[0];   v[1] = t.v[1];   v[2] = t.v[2];
        return (*this);
    }
    // membres :
    unsigned int v[3];
};


struct Mesh {
    std::vector< Vec3 > vertices;
    std::vector< Vec3 > normals;
    std::vector< Triangle > triangles;
    std::vector< Vec3 > colors;
};

Mesh mesh;

//Meshes to generate
Mesh unit_sphere;
Mesh tesselation;

bool display_normals;
bool display_hermit;
bool display_curve;
bool display_loaded_mesh;
bool display_unit_sphere;
bool display_tesselation;
bool display_bezier_bernstein;
bool display_bezier_casteljau;

// -------------------------------------------
// OpenGL/GLUT application code.
// -------------------------------------------

static GLint window;
static unsigned int SCREENWIDTH = 1600;
static unsigned int SCREENHEIGHT = 900;
static Camera camera;
static bool mouseRotatePressed = false;
static bool mouseMovePressed = false;
static bool mouseZoomPressed = false;
static int lastX=0, lastY=0, lastZoom=0;
static bool fullScreen = false;
static FastNoiseLite noise;


Vec3 crossProduct(Vec3& a, Vec3& b) {
    return Vec3(
        a[1] * b[2] - a[2] * b[1],
        a[2] * b[0] - a[0] * b[2],
        a[0] * b[1] - a[1] * b[0]
    );
}



void DrawCurve( Vec3 TabPointsOfCurve[], long nbPoints, Vec3 color ) {
    glBegin(GL_LINE_STRIP);
    glColor3f(color[0], color[1], color[2]);
    for (unsigned int j = 0 ; j < nbPoints ; ++j ) {
        glVertex3f( TabPointsOfCurve[j][0] , TabPointsOfCurve[j][1] , TabPointsOfCurve[j][2] );
    }
    glEnd();
}

void DrawPts(Vec3 TabPointsOfCurve[], long nbPoints, Vec3 color ){
    glPointSize(10.0f);
    glBegin(GL_POINTS); 
    glColor3f(color[0], color[1], color[2]);
    for (unsigned int j = 0 ; j < nbPoints ; ++j ) {
        glVertex3f( TabPointsOfCurve[j][0] , TabPointsOfCurve[j][1] , TabPointsOfCurve[j][2] );
    }
    glEnd();
}


void HermiteCubicCurve (Vec3* points,Vec3 p0, Vec3 p1, Vec3 v0, Vec3 v1, long nbU){
    for (int i = 0 ; i < nbU; i++ ) {

        float j = float(i)/(nbU-1);

        float F1 = 2*pow(j, 3) - 3*pow(j, 2) + 1;
        float F2 = -2*pow(j, 3) + 3*pow(j, 2);
        float F3 = pow(j, 3) - 2*pow(j, 2) + j;
        float F4 = pow(j, 3) - pow(j, 2);

        Vec3 position = F1*p0 + F2*p1 + F3*v0 + F4*v1;

        points[i] = position;
    }

}

long factorial(const int n) {
    long f = 1;
    for (int i=1; i<=n; ++i)
        f *= i;
    return f;
}

void BezierCurveByBernstein (Vec3* pts,Vec3 pts_ctrl[], long nb_pts_ctrl, long nbU){
    int n = nb_pts_ctrl-1;
    float Bu;
    for (int j = 0 ; j < nbU ; j++) {
        float u = float(j)/(nbU-1);

        Vec3 somme = Vec3(0,0,0);
        
        for (int i = 0 ; i < nb_pts_ctrl; i++ ) {
            Bu = ((factorial(n) / (factorial(i)*factorial(n - i))) * pow(u,i) * pow((1-u), n-i));
            somme = somme + (  Bu * pts_ctrl[i] );
        }

        pts[j] = somme;
    }

}

void BezierCurveByCasteljau(Vec3* pts,Vec3 pts_ctrl[], long nb_pts_ctrl, long nbU){
    int n = nb_pts_ctrl-1;
    float Bu;

    for (int j = 0 ; j < nbU ; j++) {
        float u = float(j)/(nbU-1);
    
        Vec3 temp_pts[nb_pts_ctrl];

        for (int i= 0; i< nb_pts_ctrl; i++) {
            temp_pts[i] = pts_ctrl[i];
        }
        
        for (int r = 1; r <= n; r++) {
            for (int i = 0; i <= n - r; i++) {
                temp_pts[i] =  (1 - u)* temp_pts[i] + u * temp_pts[i + 1] ;
            }
        }
        
        pts[j] = temp_pts[0];
    
    }
}





bool saveOFF( const std::string & filename ,
              std::vector< Vec3 > & i_vertices ,
              std::vector< Vec3 > & i_normals ,
              std::vector< Triangle > & i_triangles,
              bool save_normals = true ) {
    std::ofstream myfile;
    myfile.open(filename.c_str());
    if (!myfile.is_open()) {
        std::cout << filename << " cannot be opened" << std::endl;
        return false;
    }

    myfile << "OFF" << std::endl ;

    unsigned int n_vertices = i_vertices.size() , n_triangles = i_triangles.size();
    myfile << n_vertices << " " << n_triangles << " 0" << std::endl;

    for( unsigned int v = 0 ; v < n_vertices ; ++v ) {
        myfile << i_vertices[v][0] << " " << i_vertices[v][1] << " " << i_vertices[v][2] << " ";
        if (save_normals) myfile << i_normals[v][0] << " " << i_normals[v][1] << " " << i_normals[v][2] << std::endl;
        else myfile << std::endl;
    }
    for( unsigned int f = 0 ; f < n_triangles ; ++f ) {
        myfile << 3 << " " << i_triangles[f][0] << " " << i_triangles[f][1] << " " << i_triangles[f][2];
        myfile << std::endl;
    }
    myfile.close();
    return true;
}

void openOFF( std::string const & filename,
              std::vector<Vec3> & o_vertices,
              std::vector<Vec3> & o_normals,
              std::vector< Triangle > & o_triangles,
              bool load_normals = true )
{
    std::ifstream myfile;
    myfile.open(filename.c_str());
    if (!myfile.is_open())
    {
        std::cout << filename << " cannot be opened" << std::endl;
        return;
    }

    std::string magic_s;

    myfile >> magic_s;

    if( magic_s != "OFF" )
    {
        std::cout << magic_s << " != OFF :   We handle ONLY *.off files." << std::endl;
        myfile.close();
        exit(1);
    }

    int n_vertices , n_faces , dummy_int;
    myfile >> n_vertices >> n_faces >> dummy_int;

    o_vertices.clear();
    o_normals.clear();

    for( int v = 0 ; v < n_vertices ; ++v )
    {
        float x , y , z ;

        myfile >> x >> y >> z ;
        o_vertices.push_back( Vec3( x , y , z ) );

        if( load_normals ) {
            myfile >> x >> y >> z;
            o_normals.push_back( Vec3( x , y , z ) );
        }
    }

    o_triangles.clear();
    for( int f = 0 ; f < n_faces ; ++f )
    {
        int n_vertices_on_face;
        myfile >> n_vertices_on_face;

        if( n_vertices_on_face == 3 )
        {
            unsigned int _v1 , _v2 , _v3;
            myfile >> _v1 >> _v2 >> _v3;

            o_triangles.push_back(Triangle( _v1, _v2, _v3 ));
        }
        else if( n_vertices_on_face == 4 )
        {
            unsigned int _v1 , _v2 , _v3 , _v4;
            myfile >> _v1 >> _v2 >> _v3 >> _v4;

            o_triangles.push_back(Triangle(_v1, _v2, _v3 ));
            o_triangles.push_back(Triangle(_v1, _v3, _v4));
        }
        else
        {
            std::cout << "We handle ONLY *.off files with 3 or 4 vertices per face" << std::endl;
            myfile.close();
            exit(1);
        }
    }

}


// ------------------------------------

void initLight () {
    GLfloat light_position1[4] = {22.0f, 16.0f, 50.0f, 0.0f};
    GLfloat direction1[3] = {-52.0f,-16.0f,-50.0f};
    GLfloat color1[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    GLfloat ambient[4] = {0.3f, 0.3f, 0.3f, 0.5f};

    glLightfv (GL_LIGHT1, GL_POSITION, light_position1);
    glLightfv (GL_LIGHT1, GL_SPOT_DIRECTION, direction1);
    glLightfv (GL_LIGHT1, GL_DIFFUSE, color1);
    glLightfv (GL_LIGHT1, GL_SPECULAR, color1);
    glLightModelfv (GL_LIGHT_MODEL_AMBIENT, ambient);
    glEnable (GL_LIGHT1);
    glEnable (GL_LIGHTING);
}

void init () {
    camera.resize (SCREENWIDTH, SCREENHEIGHT);
    initLight ();
    glCullFace (GL_BACK);
    glEnable (GL_CULL_FACE);
    glDepthFunc (GL_LESS);
    glEnable (GL_DEPTH_TEST);
    glClearColor (0.2f, 0.2f, 0.3f, 1.0f);
    glEnable(GL_COLOR_MATERIAL);

    display_normals = false;
    display_curve = true;
    display_hermit = false;
    display_unit_sphere = false;
    display_loaded_mesh = false;
    display_tesselation = false;
    display_bezier_bernstein = false;
    display_bezier_bernstein = false;
}




// ------------------------------------
// rendering.
// ------------------------------------


void drawVector( Vec3 const & i_from, Vec3 const & i_to ) {

    glBegin(GL_LINES);
    glVertex3f( i_from[0] , i_from[1] , i_from[2] );
    glVertex3f( i_to[0] , i_to[1] , i_to[2] );
    glEnd();
}

void drawTriangleMesh( Mesh const & i_mesh ) {

    if(i_mesh.triangles.size() > 0){
        glBegin(GL_TRIANGLES);
        bool okNormals = (i_mesh.normals.size() == i_mesh.vertices.size());
        bool okColors = (i_mesh.colors.size() == i_mesh.vertices.size());

        Vec3 p0, p1, p2;
        Vec3 n0, n1, n2;
        Vec3 c0 = Vec3(1, 1, 1), c1 = Vec3(1, 1, 1), c2 = Vec3(1, 1, 1);
        for(unsigned int tIt = 0 ; tIt < i_mesh.triangles.size(); ++tIt) {
            p0 = i_mesh.vertices[i_mesh.triangles[tIt][0]];
            p1 = i_mesh.vertices[i_mesh.triangles[tIt][1]];
            p2 = i_mesh.vertices[i_mesh.triangles[tIt][2]];

            if (okNormals) {
                n0 = i_mesh.normals[i_mesh.triangles[tIt][0]];
                n1 = i_mesh.normals[i_mesh.triangles[tIt][1]];
                n2 = i_mesh.normals[i_mesh.triangles[tIt][2]];
            }
            if (okColors) {
                c0 = i_mesh.colors[i_mesh.triangles[tIt][0]];
                c1 = i_mesh.colors[i_mesh.triangles[tIt][1]];
                c2 = i_mesh.colors[i_mesh.triangles[tIt][2]];
            }

            glNormal3f( n0[0] , n0[1] , n0[2] );
            glColor3f( c0[0], c0[1], c0[2]);
            glVertex3f( p0[0] , p0[1] , p0[2] );
            
            glNormal3f( n1[0] , n1[1] , n1[2] );
            glColor3f( c1[0], c1[1], c1[2]);
            glVertex3f( p1[0] , p1[1] , p1[2] );
            
            glNormal3f( n2[0] , n2[1] , n2[2] );
            glColor3f( c2[0], c2[1], c2[2]);
            glVertex3f( p2[0] , p2[1] , p2[2] );
        }
        glEnd();
    }else {
        glBegin(GL_POINTS);
        for (unsigned int i = 0;  i < i_mesh.vertices.size(); i++) {
            Vec3 p = i_mesh.vertices[i];
            Vec3 c = i_mesh.colors[i];

            glColor3f(c[0], c[1], c[2]);
            glVertex3f(p[0], p[1], p[2]);
        }
        glEnd();
    }

    if(display_normals){
        glLineWidth(1.);
        glColor3f(1.,0.,0.);
        for(unsigned int pIt = 0 ; pIt < i_mesh.normals.size() ; ++pIt) {
            Vec3 to = i_mesh.vertices[pIt] + 0.02*i_mesh.normals[pIt];
            drawVector(i_mesh.vertices[pIt], to);
        }

    }

}


void draw () {

    if( display_unit_sphere ){
        glColor3f(0.8,1,0.8);
        drawTriangleMesh(unit_sphere);
    }

    if( display_tesselation ){
        glColor3f(0.8,1,0.8);
        drawTriangleMesh(tesselation);
    }

    if( display_loaded_mesh ){
        glColor3f(0.8,0.8,1);
        drawTriangleMesh(mesh);
    }

    if(display_curve){
        Vec3 points[3] = {
            Vec3(0., 0., 0.),
            Vec3(0.5, 1., 0.),
            Vec3(1., -1., 0.)
        };
        DrawCurve(points, 3, Vec3(0.,1.,0.));

        DrawPts(points, 3, Vec3(1.,1.,1.));
    }

    if(display_hermit){
        Vec3 p0 = Vec3 (0., 0., 0.);
        Vec3 p1 = Vec3 (2., 0., 0.);

        Vec3 v0 = Vec3 (1., 1., 0.);
        Vec3 v1 = Vec3 (1., -1., 0.);

        Vec3 pts[2] = {
            Vec3(0., 0., 0.),
            Vec3 (2., 0., 0.)
        };

        int nbU = 50;

        Vec3* points = new Vec3[nbU];

        HermiteCubicCurve(&points[0], pts[0],pts[1],v0,v1, nbU);

        DrawCurve(points, nbU, Vec3(1.,0.,0.));

        DrawPts(pts, 2, Vec3(1.,1.,1.));

    }

    if(display_bezier_bernstein){
        Vec3 pts_ctrls[4] = {
            Vec3(0., 0., 0.),
            Vec3(0.5, 1, 0.),
            Vec3(0.5, -0.5, 0.),
            Vec3(1, 0., 0.)
        };

        int nbU = 50;

        Vec3* points = new Vec3[nbU];

        BezierCurveByBernstein(&points[0], pts_ctrls, 4 , nbU);
        
        DrawCurve(pts_ctrls, 4,Vec3(0.,1.,0.));

        DrawCurve(points, nbU, Vec3(1.0,0.,0.));

        DrawPts(pts_ctrls, 4, Vec3(1.,1.,1.));
    }

    if(display_bezier_casteljau){
        /*
        Vec3 pts_ctrls[4] = {
            Vec3(0., 0., 0.),
            Vec3(2, 0., 0.),
            Vec3(0.5, -1, 0.),
            Vec3(0.5, 0.5, 0.)
        };
        */

        Vec3 pts_ctrls[5] = {
            Vec3(0., 0., 0.),
            Vec3(0.5, -1, 0.),
            Vec3(0.5, 0.5, 0.),
            Vec3(1, -0.3, 0.),
            Vec3(2, 0., 0.)
        };
        int nbU = 20;

        Vec3* points = new Vec3[nbU];

        BezierCurveByCasteljau(&points[0], pts_ctrls, 5 , nbU);
        
        DrawCurve(pts_ctrls, 5,Vec3(0.,1.,0.));

        DrawCurve(points, nbU, Vec3(1.0,0.,0.));

        DrawPts(pts_ctrls, 5, Vec3(1.,1.,1.));

        //DrawPts(points, nbU, Vec3(1.,1.,1.));
    }

}


void display () {
    glLoadIdentity ();
    glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // vide le buffer pour savoir qui est devant ou derrière
    camera.apply ();
    draw ();
    glFlush ();
    glutSwapBuffers ();
}

void idle () {
    glutPostRedisplay ();
}

void key (unsigned char keyPressed, int x, int y) {
    switch (keyPressed) {
    case 'f':
        if (fullScreen == true) {
            glutReshapeWindow (SCREENWIDTH, SCREENHEIGHT);
            fullScreen = false;
        } else {
            glutFullScreen ();
            fullScreen = true;
        }
        break;


    case 'w':
        GLint polygonMode[2];
        glGetIntegerv(GL_POLYGON_MODE, polygonMode);
        if(polygonMode[0] != GL_FILL)
            glPolygonMode (GL_FRONT_AND_BACK, GL_FILL);
        else
            glPolygonMode (GL_FRONT_AND_BACK, GL_LINE);
        break;


    case '1': //Press n key to display normals
        display_curve = !display_curve;
        break;

    case '2': //Press n key to display normals
        display_hermit = !display_hermit;
        break;

    case '3': //Press n key to display normals
        display_bezier_bernstein = !display_bezier_bernstein;
        break;

    case '4': //Press n key to display normals
        display_bezier_casteljau = !display_bezier_casteljau;
        break;


    default:
        break;
    }
    idle ();
}

void mouse (int button, int state, int x, int y) {
    if (state == GLUT_UP) {
        mouseMovePressed = false;
        mouseRotatePressed = false;
        mouseZoomPressed = false;
    } else {
        if (button == GLUT_LEFT_BUTTON) {
            camera.beginRotate (x, y);
            mouseMovePressed = false;
            mouseRotatePressed = true;
            mouseZoomPressed = false;
        } else if (button == GLUT_RIGHT_BUTTON) {
            lastX = x;
            lastY = y;
            mouseMovePressed = true;
            mouseRotatePressed = false;
            mouseZoomPressed = false;
        } else if (button == GLUT_MIDDLE_BUTTON) {
            if (mouseZoomPressed == false) {
                lastZoom = y;
                mouseMovePressed = false;
                mouseRotatePressed = false;
                mouseZoomPressed = true;
            }
        }
    }
    idle ();
}

void motion (int x, int y) {
    if (mouseRotatePressed == true) {
        camera.rotate (x, y);
    }
    else if (mouseMovePressed == true) {
        camera.move ((x-lastX)/static_cast<float>(SCREENWIDTH), (lastY-y)/static_cast<float>(SCREENHEIGHT), 0.0);
        lastX = x;
        lastY = y;
    }
    else if (mouseZoomPressed == true) {
        camera.zoom (float (y-lastZoom)/SCREENHEIGHT);
        lastZoom = y;
    }
}


void reshape(int w, int h) {
    camera.resize (w, h);
}



int main (int argc, char ** argv) {
    std::cout << "Starting the program..." << std::endl;

    if (argc > 2) {
        exit (EXIT_FAILURE);
    }
    glutInit (&argc, argv);
    glutInitDisplayMode (GLUT_RGBA | GLUT_DEPTH | GLUT_DOUBLE);
    glutInitWindowSize (SCREENWIDTH, SCREENHEIGHT);
    window = glutCreateWindow ("TP HAI714I");

    init ();
    glutIdleFunc (idle);
    glutDisplayFunc (display);
    glutKeyboardFunc (key);
    glutReshapeFunc (reshape);
    glutMotionFunc (motion);
    glutMouseFunc (mouse);
    key ('?', 0, 0);

    glutMainLoop ();
    return EXIT_SUCCESS;
}

