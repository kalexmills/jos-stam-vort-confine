/*
  ======================================================================
   demo.c --- protoype to show off the simple solver
  ----------------------------------------------------------------------
   Author : Jos Stam (jstam@aw.sgi.com)
   Creation Date : Jan 9 2003

   Description:

	This code is a simple prototype that demonstrates how to use the
	code provided in my GDC2003 paper entitles "Real-Time Fluid Dynamics
	for Games". This code uses OpenGL and GLUT for graphics and interface

  =======================================================================
*/

#include <stdlib.h>
#include <stdio.h>
#include <GL/glut.h>

/* macros */

#define SWAP(x0,x) {float * tmp=x0;x0=x;x=tmp;}
#define IX(i,j) ((i)+(N+2)*(j))

/* external definitions (from solver.c) */

extern void fear_step ( int N, float * x, float * u, float * u0, float * v, float * v0, float * y, float fear, 
                        float dt );
extern void dens_step ( int N, float * x, float * x0, float * u, float * v, float diff, float dt );
#ifdef _VORTICIAL_CONFINEMENT_
extern void vel_step ( int N, float * u, float * v, float * u0, float * v0, float * curl, float visc, float dt );
#else
extern void vel_step ( int N, float * u, float * v, float * u0, float * v0, float visc, float dt );
#endif

#define ELVES 0
#define ORCS 1
#define NUM_SPECIES 2

/* global variables */

static int N;
static float dt, diff, visc;
static float force, source;
static int dvel;

static int place_type = ELVES;

#ifdef _VORTICIAL_CONFINEMENT_
static float * curl;
#endif

static float * ue, * ve, * ue_prev, * ve_prev;
static float * elf, * elf_prev;

static float * uo, * vo, * uo_prev, * vo_prev;
static float * orc, * orc_prev;

static int win_id;
static int win_x, win_y;
static int mouse_down[3];
static int omx, omy, mx, my;

static float orc_fear;
static float elf_fear;

/*
  ----------------------------------------------------------------------
   free/clear/allocate simulation data
  ----------------------------------------------------------------------
*/


static void free_data ( void )
{
	if ( ue ) free ( ue );
	if ( ve ) free ( ve );
	if ( ue_prev ) free ( ue_prev );
	if ( ve_prev ) free ( ve_prev );
	if ( elf ) free ( elf );
	if ( elf_prev ) free ( elf_prev );
	if ( uo ) free ( uo );
	if ( vo ) free ( vo );
	if ( uo_prev ) free ( uo_prev );
	if ( vo_prev ) free ( vo_prev );
	if ( orc ) free ( orc );
	if ( orc_prev ) free ( orc_prev );
}

static void clear_data ( void )
{
	int i, size=(N+2)*(N+2);

	for ( i=0 ; i<size ; i++ ) {
		ue[i] = ve[i] = ue_prev[i] = ve_prev[i] = elf[i] = elf_prev[i] = 0.0f;
		uo[i] = vo[i] = uo_prev[i] = vo_prev[i] = orc[i] = orc_prev[i] = 0.0f;
	}
}

static int allocate_data ( void )
{
	int size = (N+2)*(N+2);

	ue			= (float *) malloc ( size*sizeof(float) );
	ve			= (float *) malloc ( size*sizeof(float) );
	ue_prev		= (float *) malloc ( size*sizeof(float) );
	ve_prev		= (float *) malloc ( size*sizeof(float) );
	elf		= (float *) malloc ( size*sizeof(float) );	
	elf_prev	= (float *) malloc ( size*sizeof(float) );
	uo			= (float *) malloc ( size*sizeof(float) );
	vo			= (float *) malloc ( size*sizeof(float) );
	uo_prev		= (float *) malloc ( size*sizeof(float) );
	vo_prev		= (float *) malloc ( size*sizeof(float) );
	orc		= (float *) malloc ( size*sizeof(float) );	
	orc_prev	= (float *) malloc ( size*sizeof(float) );
#ifdef _VORTICIAL_CONFINEMENT_
	curl		= (float *) malloc ( size*sizeof(float) );
#endif

	if ( !ue || !ve || !ue_prev || !ve_prev || !elf || !elf_prev ||
	     !uo || !vo || !uo_prev || !vo_prev || !orc || !orc_prev ) {
		fprintf ( stderr, "cannot allocate data\n" );
		return ( 0 );
	}

	return ( 1 );
}


/*
  ----------------------------------------------------------------------
   OpenGL specific drawing routines
  ----------------------------------------------------------------------
*/

static void pre_display ( void )
{
	glViewport ( 0, 0, win_x, win_y );
	glMatrixMode ( GL_PROJECTION );
	glLoadIdentity ();
	gluOrtho2D ( 0.0, 1.0, 0.0, 1.0 );
	glClearColor ( 0.0f, 0.0f, 0.0f, 1.0f );
	glClear ( GL_COLOR_BUFFER_BIT );
}

static void post_display ( void )
{
	glutSwapBuffers ();
}

#define SCALE 100
static void draw_velocity ( void )
{
	int i, j;
	float x, y, h;

	h = 1.0f/N * SCALE;


	glLineWidth ( 1.0f );
	glBegin ( GL_LINES );

	// Draw orc forces
	glColor3f ( 1.0f, 0.0f, 0.0f );

		for ( i=1 ; i<=N ; i++ ) {
			x = (i-0.5f)*h;
			for ( j=1 ; j<=N ; j++ ) {
				y = (j-0.5f)*h;

				glVertex2f ( x, y );
				glVertex2f ( x+uo[IX(i,j)], y+vo[IX(i,j)] );
			}
		}

	// Draw elf forces
	glColor3f ( 0.0f, 1.0f, 0.0f );

		for ( i=1 ; i<=N ; i++ ) {
			x = (i-0.5f)*h;
			for ( j=1 ; j<=N ; j++ ) {
				y = (j-0.5f)*h;

				glVertex2f ( x, y );
				glVertex2f ( x+ue[IX(i,j)], y+ve[IX(i,j)] );
			}
		}
	glEnd ();
}

static void draw_density ( void )
{
	int i, j;
	float x, y, h, e00, e01, e10, e11, o00, o01, o10, o11;

	h = 1.0f/N;

	glBegin ( GL_QUADS );

		for ( i=0 ; i<=N ; i++ ) {
			x = (i-0.5f)*h;
			for ( j=0 ; j<=N ; j++ ) {
				y = (j-0.5f)*h;

				e00 = elf[IX(i,j)];
				e01 = elf[IX(i,j+1)];
				e10 = elf[IX(i+1,j)];
				e11 = elf[IX(i+1,j+1)];

				o00 = orc[IX(i,j)];
				o01 = orc[IX(i,j+1)];
				o10 = orc[IX(i+1,j)];
				o11 = orc[IX(i+1,j+1)];

				// Draw elves in green, orcs in red.
				glColor3f ( o00, e00, 0 ); glVertex2f ( x, y );
				glColor3f ( o10, e10, 0 ); glVertex2f ( x+h, y );
				glColor3f ( o11, e11, 0 ); glVertex2f ( x+h, y+h );
				glColor3f ( o01, e01, 0 ); glVertex2f ( x, y+h );

			}
		}

	glEnd ();
}

/*
  ----------------------------------------------------------------------
   relates mouse movements to forces sources
  ----------------------------------------------------------------------
*/

static void get_from_UI ( float * d, float * u, float * v )
{
	int i, j;

	if ( !mouse_down[0] && !mouse_down[2] ) return;

	i = (int)((       mx /(float)win_x)*N+1);
	j = (int)(((win_y-my)/(float)win_y)*N+1);

	if ( i<1 || i>N || j<1 || j>N ) return;

	if ( mouse_down[0] ) {
		u[IX(i,j)] = force * (mx-omx);
		v[IX(i,j)] = force * (omy-my);
	}

	if ( mouse_down[2] ) {
		d[IX(i,j)] = source;
	}

	omx = mx;
	omy = my;

	return;
}

/*
  ----------------------------------------------------------------------
   GLUT callback routines
  ----------------------------------------------------------------------
*/

static void key_func ( unsigned char key, int x, int y )
{
	switch ( key )
	{
		case 'c':
		case 'C':
			clear_data ();
			break;

		case 'q':
		case 'Q':
			free_data ();
			exit ( 0 );
			break;

		case 'v':
		case 'V':
			dvel = !dvel;
			break;
		case 'r':
		case 'R':
			place_type = (place_type + 1) % NUM_SPECIES;
			break;
	}
}

static void mouse_func ( int button, int state, int x, int y )
{
	omx = mx = x;
	omx = my = y;

	mouse_down[button] = state == GLUT_DOWN;
}

static void motion_func ( int x, int y )
{
	mx = x;
	my = y;
}

static void reshape_func ( int width, int height )
{
	glutSetWindow ( win_id );
	glutReshapeWindow ( width, height );

	win_x = width;
	win_y = height;
}

static void idle_func ( void )
{
	int size = (N+2)*(N+2);

	for (int i=0 ; i<size ; i++ ) {
		orc_prev[i] = uo_prev[i] = vo_prev[i] = 0.0f;
		elf_prev[i] = ue_prev[i] = ve_prev[i] = 0.0f;
	}

	switch (place_type) {
	  case ORCS:
		get_from_UI ( orc_prev, uo_prev, vo_prev );
		break;
	  case ELVES:
		get_from_UI ( elf_prev, ue_prev, ve_prev );
		break;
	}
	
	// Orcs repel elves and vice-versa.
	fear_step( N, elf, ue, ue_prev, ve, ve_prev, orc, elf_fear, dt);  
	fear_step( N, orc, uo, uo_prev, vo, vo_prev, elf, orc_fear, dt);
	SWAP(ue, ue_prev); SWAP(uo, uo_prev);
	SWAP(ve, ve_prev); SWAP(vo, vo_prev);

	// Societal cohesion
	fear_step( N, elf, ue, ue_prev, ve, ve_prev, elf, 0.05, dt); 
	fear_step( N, orc, uo, uo_prev, vo, vo_prev, orc, 0.05, dt); 
	SWAP(ue, ue_prev); SWAP(uo, uo_prev);
	SWAP(ve, ve_prev); SWAP(vo, vo_prev);
	
#ifdef _VORTICIAL_CONFINEMENT_
	vel_step ( N, ue, ve, ue_prev, ve_prev, curl, visc, dt );
	vel_step ( N, uo, vo, uo_prev, vo_prev, curl, visc, dt );
#else
	vel_step ( N, ue, ve, ue_prev, ve_prev, visc, dt );
	vel_step ( N, uo, vo, uo_prev, vo_prev, visc, dt );
#endif
	dens_step ( N, elf, elf_prev, ue, ve, diff, dt );
	dens_step ( N, orc, orc_prev, uo, vo, diff, dt );

	glutSetWindow ( win_id );
	glutPostRedisplay ();
}

static void display_func ( void )
{
	pre_display ();

		if ( dvel ) draw_velocity ();
		else		draw_density ();

	post_display ();
}


/*
  ----------------------------------------------------------------------
   open_glut_window --- open a glut compatible window and set callbacks
  ----------------------------------------------------------------------
*/

static void open_glut_window ( void )
{
	glutInitDisplayMode ( GLUT_RGBA | GLUT_DOUBLE );

	glutInitWindowPosition ( 0, 0 );
	glutInitWindowSize ( win_x, win_y );
	win_id = glutCreateWindow ( "Alias | wavefront" );

	glClearColor ( 0.0f, 0.0f, 0.0f, 1.0f );
	glClear ( GL_COLOR_BUFFER_BIT );
	glutSwapBuffers ();
	glClear ( GL_COLOR_BUFFER_BIT );
	glutSwapBuffers ();

	pre_display ();

	glutKeyboardFunc ( key_func );
	glutMouseFunc ( mouse_func );
	glutMotionFunc ( motion_func );
	glutReshapeFunc ( reshape_func );
	glutIdleFunc ( idle_func );
	glutDisplayFunc ( display_func );
}


/*
  ----------------------------------------------------------------------
   main --- main routine
  ----------------------------------------------------------------------
*/

int main ( int argc, char ** argv )
{
	glutInit ( &argc, argv );

	if ( argc != 1 && argc != 9 ) {
		fprintf ( stderr, "usage : %s N dt diff visc force source elf_fear orc_fear\n", argv[0] );
		fprintf ( stderr, "where:\n" );\
		fprintf ( stderr, "\t N         : grid resolution\n" );
		fprintf ( stderr, "\t dt        : time step\n" );
		fprintf ( stderr, "\t diff      : diffusion rate of all densities\n" );
		fprintf ( stderr, "\t visc      : viscosity of the fluid\n" );
		fprintf ( stderr, "\t force     : scales the mouse movement that generate a force\n" );
		fprintf ( stderr, "\t source    : amount of density that will be deposited\n" );
		fprintf ( stderr, "\t elf_fear  : how much elves love / fear orcs\n" );
		fprintf ( stderr, "\t orc_fear  : how much orccs love / fear elves\n" );
		exit ( 1 );
	}

	if ( argc == 1 ) {
		N = 64;
		dt = 0.1f;
		diff = 0.0f;
		visc = 0.0f;
		force = 5.0f;
		source = 100.0f;
		elf_fear = 0.00025f;
		orc_fear = -0.0005f; // Orcs love elves, but the elves dislike them.
		fprintf ( stderr, "Using defaults : N=%d dt=%g diff=%g visc=%g force = %g source=%g elf_fear=%g, orc_fear=%g\n",
			N, dt, diff, visc, force, source, elf_fear, orc_fear );
	} else {
		N = atoi(argv[1]);
		dt = atof(argv[2]);
		diff = atof(argv[3]);
		visc = atof(argv[4]);
		force = atof(argv[5]);
		source = atof(argv[6]);
		elf_fear = atof(argv[7]);
		orc_fear = atof(argv[8]);
	}

	printf ( "\n\nHow to use this demo:\n\n" );
	printf ( "\t Add densities with the right mouse button\n" );
	printf ( "\t Push the orcs with the left mouse button and dragging the mouse\n" );
	printf ( "\t Toggle elf / orc placement with the 'r' key\n" );
	printf ( "\t Toggle density/velocity display with the 'v' key\n" );
	printf ( "\t Clear the simulation by pressing the 'c' key\n" );
	printf ( "\t Quit by pressing the 'q' key\n" );

	dvel = 0;

	if ( !allocate_data () ) exit ( 1 );
	clear_data ();

	win_x = 512;
	win_y = 512;
	open_glut_window ();

	glutMainLoop ();

	exit ( 0 );
}
