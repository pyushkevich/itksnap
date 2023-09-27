
// ========================================================================
// Triangle Drawing, draw an object described by triangles, 
//  object has to be closed
//
// inspired by Andras Kelemen's code for SH_TO_VOXEL
// that means, to 99% it is Andras's code (thanks andras, buddy)
// don't know where he got it from...
//
// May  00
//     styner -  scan-convert a manifold based on the old triangle code
//     styner -  added a lot of other functions to handle polygons instead of 
//               triangles
//  Martin Styner
// ========================================================================

#include <math.h>
#include <stdlib.h>       
#include <string.h>     
#include <iostream> 

using namespace std;

#include "DrawTriangles.h"

#define SGN(a)		(((a)<0) ? -1 : ((a)>0))
#define VAL3D(data, dim, i, j, k) ((data)+(dim[0]*(dim[1]*(k)+(j))+(i)) )
#define ABS(a)          (((a)<0) ? -(a) : (a))

//static void drawline(unsigned char *image, int *dim, double *p0, double *p1);

struct point {
  double x;
  double y;
};

typedef struct point Point;

struct node {
  Point *data1;
  Point *data2;
  struct node *next;
};

typedef struct node Node;
typedef struct node * PtrNode;
typedef double Point3[3];

static int Make_head(PtrNode *head);
static int Insert(PtrNode head, Point elt1, Point elt2);
static int Remove(PtrNode head, Point elt1, Point elt2);

static inline void swap(int *flag)
{ *flag = (*flag == 0) ? 1 : 0; }

void digline(int x1, int y1,  int x2,   int y2, 
	     unsigned char *image, int *dim, int color, char direction, int slice);


void 
drawBinaryPolygonsFilled(unsigned char *image, int *dim,
			 double** vertex_polygon_table,
			 int num_polygons, int *num_points_per_polygon)
//Scan converts a closed object described by a set of polygons
// the vertex_polygon_table stores all points of the num_polygons polygons
// in  num_points_per_polygon[i] the number of points of the i-th polygon has to be stored
{
  double ** vertex_triangle_table;
  int num_triangles;
  drawBinaryPolygonsFilled(image, dim,vertex_polygon_table,
			   num_polygons, num_points_per_polygon,
			   vertex_triangle_table, num_triangles);
  if (vertex_triangle_table) {
    for (int i = 0; i < num_triangles; i++) {
      delete vertex_triangle_table[i*3 + 0];
      delete vertex_triangle_table[i*3 + 1];
      delete vertex_triangle_table[i*3 + 2];
    }
    delete vertex_triangle_table;
  }
}

void 
drawBinaryPolygonsFilled(unsigned char *image, int *dim,
			 double** vertex_polygon_table,
			 int num_polygons, int *num_points_per_polygon,
			 double ** &vertex_table, int &numTriangles)  
  //Scan converts a closed object described by a set of polygons
  // the vertex_polygon_table stores all points of the num_polygons polygons
  // in  num_points_per_polygon[i] the number of points of the i-th polygon has to 
  // be stored
{
  int i, j, k;
  // convert polygons to  triangles in ray light fashion from first point
  numTriangles = 0;
  for (i = 0; i < num_polygons; i++) {
    numTriangles += num_points_per_polygon[i]-2;
    // if only 3 points -> 1 Triangle
    // for each additional point -> one more triangle
  }
  
  vertex_table = new double * [numTriangles*3];
  int pointCnt = 0, triangCnt = 0;
  double pt0[3], pt1[3];
  for (i = 0; i < num_polygons; i++) {
    for (j = 0; j < num_points_per_polygon[i]; j++) {
      for (k = 0; k < 3; k++) {
	double val = vertex_polygon_table[pointCnt][k];
	if ( j == 0 ) pt0[k] = val;
	if ( j == 1 ) pt1[k] = val;
	if ( j > 1 ) {  // make triangle
	  if (k == 0) {
	    vertex_table[triangCnt*3 + 0] = new double[3];
	    vertex_table[triangCnt*3 + 1] = new double[3];
	    vertex_table[triangCnt*3 + 2] = new double[3];
	  }
	  vertex_table[triangCnt*3 + 0][k] = pt0[k];
	  vertex_table[triangCnt*3 + 1][k] = pt1[k];
	  vertex_table[triangCnt*3 + 2][k] = val;
	  pt1[k] = val; // advance point
	}
      }
      if ( j > 1 ) triangCnt ++;
      pointCnt++;
    }
  }
  drawBinaryTrianglesFilled(image, dim, vertex_table,numTriangles, 1);
}

void 
drawBinaryTrianglesFilled(unsigned char *image, int *dim, 
			  double ** vertex_table,
			  int num_triangles, int segColor)
  // draws an object fully defined by triangles with its interior filled into image,
  // the coordinates of vertex_table are in units of voxels with 
  // image(0,0,0) as an origin
  // vertex_table is a table of num_triangles*3 Pointers to 3 doubles (a point)
{
  int i,j,k;
  int maxx, minx, maxy, miny, maxz, minz;
  Point3 p0, p1, p2; 
  Point pt1, pt2;
  PtrNode g_head, tmp;
  int inout;
  
  maxx = 0;
  minx = dim[0];
  
  // determine maxx
  for(i = 0; i < num_triangles*3; i++) {
    if(vertex_table[i][0]>maxx) maxx = (int) vertex_table[i][0];
    if(vertex_table[i][0]<minx) minx = (int) vertex_table[i][0];
  }
  
  for(int p_p=minx;p_p<=maxx;p_p++) {
    Make_head(&g_head);
    maxy = 0; maxz = 0;
    miny = dim[1]; minz = dim[2];

    for(k=0;k<num_triangles;k++) {
      
      p0[0]=vertex_table[k*3+0][0];
      p0[1]=vertex_table[k*3+0][1];
      p0[2]=vertex_table[k*3+0][2];
      p1[0]=vertex_table[k*3+1][0];
      p1[1]=vertex_table[k*3+1][1];
      p1[2]=vertex_table[k*3+1][2];
      p2[0]=vertex_table[k*3+2][0];
      p2[1]=vertex_table[k*3+2][1];
      p2[2]=vertex_table[k*3+2][2];
      
      if((SGN(p0[0]-(double)p_p)!=SGN(p1[0]-(double)p_p)) ||
	 (SGN(p1[0]-(double)p_p)!=SGN(p2[0]-(double)p_p)) ||
	 (SGN(p0[0]-(double)p_p)!=SGN(p2[0]-(double)p_p))) {
	if((SGN(p0[0]-(double)p_p)!=SGN(p1[0]-(double)p_p)) &&
	   (SGN(p1[0]-(double)p_p)!=SGN(p2[0]-(double)p_p))) {
	  pt1.x = (p_p-p0[0])*(p1[1]-p0[1])/(p1[0]-p0[0])+p0[1];
	  pt1.y = (p_p-p0[0])*(p1[2]-p0[2])/(p1[0]-p0[0])+p0[2];
	  pt2.x = (p_p-p1[0])*(p2[1]-p1[1])/(p2[0]-p1[0])+p1[1];
	  pt2.y = (p_p-p1[0])*(p2[2]-p1[2])/(p2[0]-p1[0])+p1[2];
	  if(pt1.x>maxy) maxy = (int) pt1.x;
	  if(pt1.y>maxz) maxz = (int) pt1.y;
	  if(pt1.x<miny) miny = (int) pt1.x;
	  if(pt1.y<minz) minz = (int) pt1.y;
	  if(pt2.x>maxy) maxy = (int) pt2.x;
	  if(pt2.y>maxz) maxz = (int) pt2.y;
	  if(pt2.x<miny) miny = (int) pt2.x;
	  if(pt2.y<minz) minz = (int) pt2.y;
	  if((pt1.y) != (pt2.y))
	    Insert(g_head, pt1, pt2);
	    }
	if((SGN(p1[0]-(double)p_p)!=SGN(p2[0]-(double)p_p)) &&
	   (SGN(p0[0]-(double)p_p)!=SGN(p2[0]-(double)p_p))) {
	  pt1.x = (p_p-p0[0])*(p2[1]-p0[1])/(p2[0]-p0[0])+p0[1];
	  pt1.y = (p_p-p0[0])*(p2[2]-p0[2])/(p2[0]-p0[0])+p0[2];
	  pt2.x = (p_p-p1[0])*(p2[1]-p1[1])/(p2[0]-p1[0])+p1[1];
	  pt2.y = (p_p-p1[0])*(p2[2]-p1[2])/(p2[0]-p1[0])+p1[2];
	  if(pt1.x>maxy) maxy = (int) pt1.x;
	  if(pt1.y>maxz) maxz = (int) pt1.y;
	  if(pt1.x<miny) miny = (int) pt1.x;
	  if(pt1.y<minz) minz = (int) pt1.y;
	  if(pt2.x>maxy) maxy = (int) pt2.x;
	  if(pt2.y>maxz) maxz = (int) pt2.y;
	  if(pt2.x<miny) miny = (int) pt2.x;
	  if(pt2.y<minz) minz = (int) pt2.y;
	  if((pt1.y) != (pt2.y))
	    Insert(g_head, pt1, pt2);
	}
	if((SGN(p0[0]-(double)p_p)!=SGN(p1[0]-(double)p_p)) &&
	   (SGN(p0[0]-(double)p_p)!=SGN(p2[0]-(double)p_p))) {
	  pt1.x = (p_p-p0[0])*(p1[1]-p0[1])/(p1[0]-p0[0])+p0[1];
	  pt1.y = (p_p-p0[0])*(p1[2]-p0[2])/(p1[0]-p0[0])+p0[2];
	  pt2.x = (p_p-p0[0])*(p2[1]-p0[1])/(p2[0]-p0[0])+p0[1];
	  pt2.y = (p_p-p0[0])*(p2[2]-p0[2])/(p2[0]-p0[0])+p0[2];
	  if(pt1.x>maxy) maxy = (int) pt1.x;
	  if(pt1.y>maxz) maxz = (int) pt1.y;
	  if(pt1.x<miny) miny = (int) pt1.x;
	  if(pt1.y<minz) minz = (int) pt1.y;
	  if(pt2.x>maxy) maxy = (int) pt2.x;
	  if(pt2.y>maxz) maxz = (int) pt2.y;
	  if(pt2.x<miny) miny = (int) pt2.x;
	  if(pt2.y<minz) minz = (int) pt2.y;
	  if((pt1.y) != (pt2.y))
	    Insert(g_head, pt1, pt2);
	}
      }
    }

    for(j=minz;j<=maxz;j++) {
      for(i=miny;i<=maxy;i++) {
      inout = 0;
	if (g_head != NULL) {
	  tmp = g_head->next;
	  while (tmp != NULL) {
	    if((((tmp->data1->y)<=(double) j) != 
		 ((tmp->data2->y)<=(double) j)) && 
	       (((tmp->data1->x)<(double) i) && 
		((tmp->data2->x)<(double) i)))
 	      swap(&inout); 
	    tmp = tmp->next;
	  }
	}
 	if (inout == 1) {
 	  (VAL3D(image, dim, p_p, i, j))[0] = segColor; 
	}
      }
    }
    // RemoveAll
    if (g_head != NULL) {
      tmp = g_head->next;
      PtrNode tmp2;
      while (tmp != NULL) {
	tmp2 = tmp->next;
	free( tmp->data1 );
	free( tmp->data2 );
	free( tmp );
	tmp = tmp2;
      }
      tmp = g_head;
      //Remove g_head;
      free( tmp->data1 );
      free( tmp->data2 );
      free( tmp );
    }
  } 
  
}

void 
drawBinaryPolygonsSheetFilled(unsigned char *image, int *dim,
			      double** vertex_polygon_table,
			      int num_polygons, int *num_points_per_polygon)
  //Scan converts a manifold object described by a set of polygons
  // the vertex_polygon_table stores all points of the num_polygons polygons
  // in  num_points_per_polygon[i] the number of points of the i-th polygon has to be stored
{
  double ** vertex_triangle_table;
  int num_triangles;
  drawBinaryPolygonsSheetFilled(image, dim,vertex_polygon_table,
				num_polygons, num_points_per_polygon,
				vertex_triangle_table, num_triangles);
  if (vertex_triangle_table) {
    for (int i = 0; i < num_triangles; i++) {
      delete vertex_triangle_table[i*3 + 0];
      delete vertex_triangle_table[i*3 + 1];
      delete vertex_triangle_table[i*3 + 2];
    }
    delete vertex_triangle_table;
  }
}
  
void 
drawBinaryPolygonsSheetFilled(unsigned char *image, int *dim,
			      double** vertex_polygon_table,
			      int num_polygons, int *num_points_per_polygon,
			      double ** &vertex_table, int &numTriangles)  
  //Scan converts a manifold object described by a set of polygons
  // the vertex_polygon_table stores all points of the num_polygons polygons
  // in  num_points_per_polygon[i] the number of points of the i-th polygon has to be stored
{
  int i, j, k;
  // convert polygons to  triangles in ray light fashion from first point
  numTriangles = 0;
  for (i = 0; i < num_polygons; i++) {
    numTriangles += num_points_per_polygon[i]-2;
    // if only 3 points -> 1 Triangle
    // for each additional point -> one more triangle
  }
  
  vertex_table = new double * [numTriangles*3];
  int pointCnt = 0, triangCnt = 0;
  double pt0[3], pt1[3];
  for (i = 0; i < num_polygons; i++) {
    for (j = 0; j < num_points_per_polygon[i]; j++) {
      for (k = 0; k < 3; k++) {
	double val = vertex_polygon_table[pointCnt][k];
	if ( j == 0 ) pt0[k] = val;
	if ( j == 1 ) pt1[k] = val;
	if ( j > 1 ) {  // make triangle
	  if (k == 0) {
	    vertex_table[triangCnt*3 + 0] = new double[3];
	    vertex_table[triangCnt*3 + 1] = new double[3];
	    vertex_table[triangCnt*3 + 2] = new double[3];
	  }
	  vertex_table[triangCnt*3 + 0][k] = pt0[k];
	  vertex_table[triangCnt*3 + 1][k] = pt1[k];
	  vertex_table[triangCnt*3 + 2][k] = val;
	  pt1[k] = val; // advance point
	}
      }
      if ( j > 1 ) triangCnt ++;
      pointCnt++;
    }
  }
  drawBinaryTrianglesSheetFilled(image, dim, vertex_table,numTriangles, 1);
}

void 
drawBinaryTrianglesSheetFilled(unsigned char *image, int *dim, 
                               double ** vertex_table,
                               int num_triangles, int segColor)
//Scan converts a manifold object described by a set of triangles
// the coordinates of vertex_table are in units of voxels with
// image(0,0,0) as an origin
// vertex_table is a table of num_triangles*3 Pointers to 3 doubles (a point)
  {
  int i,k,p_p;
  int maxz, minz, maxy,miny, maxx, minx;
  Point3 p0, p1, p2;
  //  int color = 255;
  
  maxz = 0;
  minz = dim[2];
  
  // Draw twice, once in z and then in y, because triangles parrallel to scan
  // plane won't be drawn otherwise
  
  // determine maxz
  for(i = 0; i < num_triangles*3; i++) {
    if(vertex_table[i][2]>maxz) maxz = (int) vertex_table[i][2];
    if(vertex_table[i][2]<minz) minz = (int) vertex_table[i][2];
    }
  for(p_p=minz;p_p<=maxz;p_p++) { // Zcoor

    for(k=0;k<num_triangles;k++) {
      
      p0[0]=vertex_table[k*3+0][0];
      p0[1]=vertex_table[k*3+0][1];
      p0[2]=vertex_table[k*3+0][2];
      p1[0]=vertex_table[k*3+1][0];
      p1[1]=vertex_table[k*3+1][1];
      p1[2]=vertex_table[k*3+1][2];
      p2[0]=vertex_table[k*3+2][0];
      p2[1]=vertex_table[k*3+2][1];
      p2[2]=vertex_table[k*3+2][2];

      double x1,y1,x2,y2;
      
      if((SGN(p0[2]-(float)p_p)!=SGN(p1[2]-(float)p_p)) ||
         (SGN(p1[2]-(float)p_p)!=SGN(p2[2]-(float)p_p)) ||
         (SGN(p0[2]-(float)p_p)!=SGN(p2[2]-(float)p_p))) {
        if((SGN(p0[2]-(float)p_p)!=SGN(p1[2]-(float)p_p)) &&
           (SGN(p1[2]-(float)p_p)!=SGN(p2[2]-(float)p_p))) {
          x1 = (p_p-p0[2])*(p1[0]-p0[0])/(p1[2]-p0[2])+p0[0];
          y1 = (p_p-p0[2])*(p1[1]-p0[1])/(p1[2]-p0[2])+p0[1];
          x2 = (p_p-p1[2])*(p2[0]-p1[0])/(p2[2]-p1[2])+p1[0];
          y2 = (p_p-p1[2])*(p2[1]-p1[1])/(p2[2]-p1[2])+p1[1];
          digline((int)x1, (int)y1, (int)x2, (int)y2,
                  image,dim,segColor,'Z',p_p);
          }
        if((SGN(p1[2]-(float)p_p)!=SGN(p2[2]-(float)p_p)) &&
           (SGN(p0[2]-(float)p_p)!=SGN(p2[2]-(float)p_p))) {
          x1 = (p_p-p0[2])*(p2[0]-p0[0])/(p2[2]-p0[2])+p0[0];
          y1 = (p_p-p0[2])*(p2[1]-p0[1])/(p2[2]-p0[2])+p0[1];
          x2 = (p_p-p1[2])*(p2[0]-p1[0])/(p2[2]-p1[2])+p1[0];
          y2 = (p_p-p1[2])*(p2[1]-p1[1])/(p2[2]-p1[2])+p1[1];
          digline((int)x1, (int)y1, (int)x2,(int)y2,
                  image,dim,segColor,'Z',p_p);
          }
        if((SGN(p0[2]-(float)p_p)!=SGN(p1[2]-(float)p_p)) &&
           (SGN(p0[2]-(float)p_p)!=SGN(p2[2]-(float)p_p))) {
          x1 = (p_p-p0[2])*(p1[0]-p0[0])/(p1[2]-p0[2])+p0[0];
          y1 = (p_p-p0[2])*(p1[1]-p0[1])/(p1[2]-p0[2])+p0[1];
          x2 = (p_p-p0[2])*(p2[0]-p0[0])/(p2[2]-p0[2])+p0[0];
          y2 = (p_p-p0[2])*(p2[1]-p0[1])/(p2[2]-p0[2])+p0[1];
          digline((int)x1, (int)y1, (int)x2, (int)y2,
                  image,dim,segColor,'Z',p_p);
          }
        }
      }
    }
  
  maxy = 0;
  miny = dim[1];
  // determine maxy
  for(i = 0; i < num_triangles*3; i++) {
    if(vertex_table[i][1]>maxy) maxy = (int) vertex_table[i][1];
    if(vertex_table[i][1]<miny) miny = (int) vertex_table[i][1];
    }
  for(p_p=miny;p_p<=maxy;p_p++) { // Ycoor

    for(k=0;k<num_triangles;k++) {
      
      p0[0]=vertex_table[k*3+0][0];
      p0[1]=vertex_table[k*3+0][1];
      p0[2]=vertex_table[k*3+0][2];
      p1[0]=vertex_table[k*3+1][0];
      p1[1]=vertex_table[k*3+1][1];
      p1[2]=vertex_table[k*3+1][2];
      p2[0]=vertex_table[k*3+2][0];
      p2[1]=vertex_table[k*3+2][1];
      p2[2]=vertex_table[k*3+2][2];

      double x1,y1,x2,y2;
      if((SGN(p0[1]-(float)p_p)!=SGN(p1[1]-(float)p_p)) ||
         (SGN(p1[1]-(float)p_p)!=SGN(p2[1]-(float)p_p)) ||
         (SGN(p0[1]-(float)p_p)!=SGN(p2[1]-(float)p_p))) {
        if((SGN(p0[1]-(float)p_p)!=SGN(p1[1]-(float)p_p)) &&
           (SGN(p1[1]-(float)p_p)!=SGN(p2[1]-(float)p_p))) {
          x1 = (p_p-p0[1])*(p1[0]-p0[0])/(p1[1]-p0[1])+p0[0];
          y1 = (p_p-p0[1])*(p1[2]-p0[2])/(p1[1]-p0[1])+p0[2];
          x2 = (p_p-p1[1])*(p2[0]-p1[0])/(p2[1]-p1[1])+p1[0];
          y2 = (p_p-p1[1])*(p2[2]-p1[2])/(p2[1]-p1[1])+p1[2];
          digline((int)x1, (int)y1, (int)x2, (int)y2,
                  image,dim,segColor,'Y',p_p);
          }
        if((SGN(p1[1]-(float)p_p)!=SGN(p2[1]-(float)p_p)) &&
           (SGN(p0[1]-(float)p_p)!=SGN(p2[1]-(float)p_p))) {
          x1 = (p_p-p0[1])*(p2[0]-p0[0])/(p2[1]-p0[1])+p0[0];
          y1 = (p_p-p0[1])*(p2[2]-p0[2])/(p2[1]-p0[1])+p0[2];
          x2 = (p_p-p1[1])*(p2[0]-p1[0])/(p2[1]-p1[1])+p1[0];
          y2 = (p_p-p1[1])*(p2[2]-p1[2])/(p2[1]-p1[1])+p1[2];
          digline((int)x1, (int)y1, (int)x2, (int)y2,
                  image,dim,segColor,'Y',p_p);
          }
        if((SGN(p0[1]-(float)p_p)!=SGN(p1[1]-(float)p_p)) &&
           (SGN(p0[1]-(float)p_p)!=SGN(p2[1]-(float)p_p))) {
          x1 = (p_p-p0[1])*(p1[0]-p0[0])/(p1[1]-p0[1])+p0[0];
          y1 = (p_p-p0[1])*(p1[2]-p0[2])/(p1[1]-p0[1])+p0[2];
          x2 = (p_p-p0[1])*(p2[0]-p0[0])/(p2[1]-p0[1])+p0[0];
          y2 = (p_p-p0[1])*(p2[2]-p0[2])/(p2[1]-p0[1])+p0[2];
          digline((int)x1, (int)y1, (int)x2, (int)y2,
                  image,dim,segColor,'Y',p_p);
          }
        }
      }
    }

  maxx = 0;
  minx = dim[0];
  // determine maxx
  for(i = 0; i < num_triangles*3; i++) {
    if(vertex_table[i][0]>maxx) maxx = (int) vertex_table[i][0];
    if(vertex_table[i][0]<minx) minx = (int) vertex_table[i][0];
    }
  for(p_p=minx;p_p<=maxx;p_p++) { // Xcoor

    for(k=0;k<num_triangles;k++) {
      
      p0[0]=vertex_table[k*3+0][0];
      p0[1]=vertex_table[k*3+0][1];
      p0[2]=vertex_table[k*3+0][2];
      p1[0]=vertex_table[k*3+1][0];
      p1[1]=vertex_table[k*3+1][1];
      p1[2]=vertex_table[k*3+1][2];
      p2[0]=vertex_table[k*3+2][0];
      p2[1]=vertex_table[k*3+2][1];
      p2[2]=vertex_table[k*3+2][2];

      double x1,y1,x2,y2;
      if((SGN(p0[0]-(float)p_p)!=SGN(p1[0]-(float)p_p)) ||
         (SGN(p1[0]-(float)p_p)!=SGN(p2[0]-(float)p_p)) ||
         (SGN(p0[0]-(float)p_p)!=SGN(p2[0]-(float)p_p))) {
        if((SGN(p0[0]-(float)p_p)!=SGN(p1[0]-(float)p_p)) &&
           (SGN(p1[0]-(float)p_p)!=SGN(p2[0]-(float)p_p))) {
          x1 = (p_p-p0[0])*(p1[1]-p0[1])/(p1[0]-p0[0])+p0[1];
          y1 = (p_p-p0[0])*(p1[2]-p0[2])/(p1[0]-p0[0])+p0[2];
          x2 = (p_p-p1[0])*(p2[1]-p1[1])/(p2[0]-p1[0])+p1[1];
          y2 = (p_p-p1[0])*(p2[2]-p1[2])/(p2[0]-p1[0])+p1[2];
          digline((int)x1, (int)y1, (int)x2, (int)y2,
                  image,dim,segColor,'X',p_p);
          }
        if((SGN(p1[0]-(float)p_p)!=SGN(p2[0]-(float)p_p)) &&
           (SGN(p0[0]-(float)p_p)!=SGN(p2[0]-(float)p_p))) {
          x1 = (p_p-p0[0])*(p2[1]-p0[1])/(p2[0]-p0[0])+p0[1];
          y1 = (p_p-p0[0])*(p2[2]-p0[2])/(p2[0]-p0[0])+p0[2];
          x2 = (p_p-p1[0])*(p2[1]-p1[1])/(p2[0]-p1[0])+p1[1];
          y2 = (p_p-p1[0])*(p2[2]-p1[2])/(p2[0]-p1[0])+p1[2];
          digline((int)x1, (int)y1, (int)x2, (int)y2,
                  image,dim,segColor,'X',p_p);
          }
        if((SGN(p0[0]-(float)p_p)!=SGN(p1[0]-(float)p_p)) &&
           (SGN(p0[0]-(float)p_p)!=SGN(p2[0]-(float)p_p))) {
          x1 = (p_p-p0[0])*(p1[1]-p0[1])/(p1[0]-p0[0])+p0[1];
          y1 = (p_p-p0[0])*(p1[2]-p0[2])/(p1[0]-p0[0])+p0[2];
          x2 = (p_p-p0[0])*(p2[1]-p0[1])/(p2[0]-p0[0])+p0[1];
          y2 = (p_p-p0[0])*(p2[2]-p0[2])/(p2[0]-p0[0])+p0[2];
          digline((int)x1, (int)y1, (int)x2, (int)y2,
                  image,dim,segColor,'X',p_p);
          }
        }
      }
    }
  }

void
digline(int x1, int y1,  int x2,   int y2, 
        unsigned char *image, int *dim, int color, char direction, int slice)
  {
  int d, x, y, ax, ay, sx, sy, dx, dy;
  
  dx = x2-x1;  ax = ABS(dx)<<1;  sx = SGN(dx);
  dy = y2-y1;  ay = ABS(dy)<<1;  sy = SGN(dy);
  
  int offset = 0;
  int size1,size2;
  if (direction == 'Z') {
    offset = dim[0]*dim[1]*slice;
    size1 = dim[0];
    size2 = 1;
    } else if (direction == 'Y') {
    offset = dim[0]*slice;
    size1 = dim[0]*dim[1];
    size2 = 1;
    } else if (direction == 'X') {
    offset = slice;
    size1 = dim[0]*dim[1];
    size2 = dim[0];
    }
  
  
  x = x1;
  y = y1;
  if (ax>ay) {                /* x dominant */
    d = ay-(ax>>1);
    for (;;) {
      image[offset+size1*y+size2*x] = (char) (color);
      if (x==x2) return;
      if (d>=0) {
        y += sy;
        d -= ax;
        }
      x += sx;
      d += ay;
      }
    }
  else {                      /* y dominant */
    d = ax-(ay>>1);
    for (;;) {
      image[offset+size1*y+size2*x] = (char) (color);
      if (y==y2) return;
      if (d>=0) {
        x += sx;
        d -= ay;
        }
      y += sy;
      d += ax;
      }
    }
  }



// void versuch()
// {
//   for(int k=0;k<num_triangles;k++) {
//     // paint triangle k 
//     double p0[0], p1[3], p2[3];
//     for (int i = 0; i < 3; i++) {
//       p0[i] = vertex_table[k*3+0][i];
//       p1[i] = vertex_table[k*3+1][i];
//       p2[i] = vertex_table[k*3+2][i];
//     }
//     // triangle p0-p1-p2
//     double d1[3],d2[3]; // step increments
//     int steps = 0, tmp_steps = 0;
//     for (int i = 0; i < 3; i++) {
//       d1[i] = p2[i] - p0[i];
//       d2[i] = p2[i] - p1[i];
//     }
//     for (int i = 0; i < 3; i++) 
//       steps += d1[i]*d1[i];
//     for (int i = 0; i < 3; i++) 
//       tmp_steps += d2[i]*d2[i];
//     if (tmp_steps > steps) steps = tmp_steps;
//     steps = sqrt(steps); // EEEK, Sqrt in a fast drawing algorithm
//     for (int i = 0; i < 3; i++) {
//       d1[i] /=steps;
//       d2[i] /=steps;
//     }
//     for (int i = 0; i < steps; i++) {
//       // draw line
//       drawline(image,dim,p0,p1);
//       for (int i = 0; i < 3; i++) {
// 	p0[i] += d1[i];
// 	p1[i] += d2[i];
//       }
//     }
//   }
// }


/*========================================================================*/
/* List Handling for Triangle Draw                                        */
/*========================================================================*/

static int Make_head(PtrNode *head)
{
  *head = (PtrNode) calloc( 1, sizeof( Node ) );
  if( *head == NULL ) return 0;

  (*head)->next = NULL;
  return 1;
}

static int Insert(PtrNode head, Point elt1, Point elt2)
{
  PtrNode tmp;

  tmp = (PtrNode) calloc( 1, sizeof( Node ) );
  if( tmp == NULL ) return 0;
  tmp->data1 = (Point *) calloc( 1, sizeof( Point ) );
  tmp->data2 = (Point *) calloc( 1, sizeof( Point ) );
  if( tmp->data1 == NULL ) {
    return 0;
    free(tmp);
  }

  if( head->next == NULL ) {
    head->next = tmp;
    memcpy( tmp->data1, &elt1, sizeof( Point ) );
    memcpy( tmp->data2, &elt2, sizeof( Point ) );
    tmp->next = NULL;
    return 1;
  }
  else {
    tmp->next = head->next;
    head->next = tmp;
    memcpy( tmp->data1, &elt1, sizeof( Point ) );
    memcpy( tmp->data2, &elt2, sizeof( Point ) );
    return 1;
  }
}

static int Remove(PtrNode head, Point elt1, Point elt2)
{
  int flag = 0;
  PtrNode tmp;
  PtrNode prev;

  tmp = head->next;
  prev = head;

  while( tmp != NULL ) {
    
    if((elt1.x == tmp->data1->x) && 
       (elt1.y == tmp->data1->y) &&
       (elt2.x == tmp->data2->x) && 
       (elt2.y == tmp->data2->y)) {

      /* Remove it */
      prev->next = tmp->next;
      flag = 1;

      free( tmp->data1 );
      free( tmp->data2 );
      free( tmp );
      
      tmp = prev->next;
    }
    else {
      prev = tmp;
      tmp = tmp->next;
    }
  }

  return flag;
}
