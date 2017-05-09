// Copyright 2009 Isis Innovation Limited
#define GL_GLEXT_PROTOTYPES 1
#include "System.h"
#include "ARDriver.h"
#include "Map.h"
#include "Games.h"
#include "AliceScript.h"
#include <cvd/image_io.h>
#include "fbx/SceneContext.h"

namespace PTAMM {

using namespace GVars3;
using namespace CVD;
using namespace std;

static SceneContext * gSceneContext;
static	AliceScript as;
static	std::string buffer;
static float scale=1,thetaX=0.0,thetaY=0.0;





static bool CheckFramebufferStatus();

/**
 * Constructor
 * @param cam Reference to the camera
 * @param irFrameSize the size of the frame
 * @param glw the glwindow
 * @param map the current map
 */
ARDriver::ARDriver(const ATANCamera &cam, ImageRef irFrameSize, GLWindow2 &glw, Map &map)
  :mCamera(cam), mGLWindow(glw), mpMap( &map )
{
  mirFrameSize = irFrameSize;
  mCamera.SetImageSize(mirFrameSize);
  mbInitialised = false;

    const int DEFAULT_WINDOW_WIDTH = 640;
    const int DEFAULT_WINDOW_HEIGHT = 480;
    const bool lSupportVBO = InitializeOpenGL();

	gSceneContext = new SceneContext(NULL, DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT, true); 

}


/**
 * Initialize the AR driver
 */
void ARDriver::Init()
{
  mbInitialised = true;
  mirFBSize = GV3::get<ImageRef>("ARDriver.FrameBufferSize", ImageRef(1200,900), SILENT);
  glGenTextures(1, &mnFrameTex);
  glBindTexture(GL_TEXTURE_RECTANGLE_ARB,mnFrameTex);
  glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0,
	       GL_RGBA, mirFrameSize.x, mirFrameSize.y, 0,
	       GL_RGBA, GL_UNSIGNED_BYTE, NULL);
  MakeFrameBuffer();

  try {
    CVD::img_load(mLostOverlay, "ARData/Overlays/searching.png");
  }
  catch(CVD::Exceptions::All err) {
    cerr << "Failed to load searching image " << "\"ARData/Overlays/searching.png\"" << ": " << err.what << endl;
  }  
  
  try {
    CVD::img_load(mLogoOverlay, "ARData/Overlays/p8.png");
  }
  catch(CVD::Exceptions::All err) {
    cerr << "Failed to load searching image " << "\"ARData/Overlays/p8.png\"" << ": " << err.what << endl;
  }  

  GUI.RegisterCommand("chatbot", GUICommandCallBack, this);
}


/**
 * Reset the game and the frame counter
 */
void ARDriver::Reset()
{
  if(mpMap->pGame) {
    mpMap->pGame->Reset();
  }

  mnCounter = 0;
}


/**
 * Render the AR composite image
 * @param imFrame The camera frame
 * @param se3CfromW The camera position
 * @param bLost Is the camera lost
 */
void ARDriver::Render(Image<Rgb<CVD::byte> > &imFrame, SE3<> se3CfromW, bool bLost)
{
  if(!mbInitialised)
  {
    Init();
    Reset();
  };

  mse3CfromW = se3CfromW;
  mnCounter++;

  // Upload the image to our frame texture
  glBindTexture(GL_TEXTURE_RECTANGLE_ARB, mnFrameTex);
  glTexSubImage2D(GL_TEXTURE_RECTANGLE_ARB,
		  0, 0, 0,
		  mirFrameSize.x, mirFrameSize.y,
		  GL_RGB,
		  GL_UNSIGNED_BYTE,
		  imFrame.data());

  // Set up rendering to go the FBO, draw undistorted video frame into BG
  glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,mnFrameBuffer);
  CheckFramebufferStatus();
  glViewport(0,0,mirFBSize.x,mirFBSize.y);
  DrawFBBackGround();
  glClearDepth(1);
  glClear(GL_DEPTH_BUFFER_BIT);

  // Set up 3D projection
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();

  //only draw 3d stuff if not lost.
  if(!bLost)
  {
    glMultMatrix(mCamera.MakeUFBLinearFrustumMatrix(0.005, 100));
    glMultMatrix(se3CfromW);

    DrawFadingGrid();

    if(mpMap->pGame) {
      mpMap->pGame->Draw3D( mGLWindow, *mpMap, se3CfromW);
//      mpMap->pGame->state("hello");
    }
/*
    static int y=70;
    if (gSceneContext->GetStatus() == SceneContext::MUST_BE_LOADED)
    {
        gSceneContext->LoadFile();
        gSceneContext->SetCurrentAnimStack(0);
  
    }else
    {    

        static int y=70;
    switch(as.get_state()){
            case STATE_INIT:
                gSceneContext->OnTimerClick(1);
                break;
            case STATE_START:
                gSceneContext->OnTimerClick(1);
                as.gen_answer(buffer);
                //buffer="(Speaking...)";
                break;
            case STATE_MAKING:
                gSceneContext->OnTimerClick(1);
                as.finish_make();
                break;
            case STATE_STARTSPEAK:
                gSceneContext->OnTimerClick(1);
                as.gen_speak();
                break;
            case STATE_SPEAKING:                
                gSceneContext->OnTimerClick(2);
                as.finish_speak();
                buffer="";
                break;
            default:
                break;
            
        }

    }

    glPushMatrix();
    glScalef(0.0005*scale,0.0005*scale,0.0005*scale);
    gSceneContext->OnDisplay(thetaX,thetaY);   
    glPopMatrix();
*/
//  glDisable(GL_LIGHTING);
//  glDisable(GL_DEPTH_TEST);

  }

  glDisable(GL_DEPTH_TEST);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glDisable(GL_BLEND);

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  // Set up for drawing 2D stuff:
  glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,0);

  DrawDistortedFB();
  
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  mGLWindow.SetupViewport();
  mGLWindow.SetupVideoOrtho();
  mGLWindow.SetupVideoRasterPosAndZoom();

  
  //2d drawing
  if(!bLost)
  {   
    if(mpMap->pGame) {
      mpMap->pGame->Draw2D(mGLWindow, *mpMap);
    }
  }
  else
  {
    //draw the lost ar overlays
  }
    glEnable(GL_BLEND);
  //  glRasterPos2i( ( mGLWindow.size().x - mLostOverlay.size().x )/2,
  //                 ( mGLWindow.size().y - mLostOverlay.size().y )/2 );
//HBB  
    glRasterPos2i(0,75);
    glDrawPixels(mLostOverlay);

    glRasterPos2i(10,10);
    glDrawPixels(mLogoOverlay);
    glDisable(GL_BLEND);
 
}





void ARDriver::GUICommandCallBack(void* ptr, string sCommand, string sParams)
{
  Command c;
  c.sCommand = sCommand;
  c.sParams = sParams;
  ((ARDriver*) ptr)->mvQueuedCommands.push_back(c);
}


/**
 * This is called in the tracker's own thread.
 * @param sCommand command string
 * @param sParams  parameter string
 */
void ARDriver::GUICommandHandler(string sCommand, string sParams)  // Called by the callback func..
{
  if(sCommand=="chatbot")
  {
    return;
  }
}
















/**
 * Make the frame buffer
 */
void ARDriver::MakeFrameBuffer()
{
  // Needs nvidia drivers >= 97.46
  cout << "  ARDriver: Creating FBO... ";

  glGenTextures(1, &mnFrameBufferTex);
  glBindTexture(GL_TEXTURE_RECTANGLE_ARB,mnFrameBufferTex);
  glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0,
	       GL_RGBA, mirFBSize.x, mirFBSize.y, 0,
	       GL_RGBA, GL_UNSIGNED_BYTE, NULL);
  glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  GLuint DepthBuffer;
  glGenRenderbuffersEXT(1, &DepthBuffer);
  glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, DepthBuffer);
  glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT24, mirFBSize.x, mirFBSize.y);

  glGenFramebuffersEXT(1, &mnFrameBuffer);
  glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, mnFrameBuffer);
  glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
			    GL_TEXTURE_RECTANGLE_ARB, mnFrameBufferTex, 0);
  glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT,
  			       GL_RENDERBUFFER_EXT, DepthBuffer);

  CheckFramebufferStatus();
  cout << " .. created FBO." << endl;
  glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
}


/**
 * check the status of the frame buffer
 */
static bool CheckFramebufferStatus()
{
  GLenum n;
  n = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
  if(n == GL_FRAMEBUFFER_COMPLETE_EXT)
    return true; // All good
  
  cout << "glCheckFrameBufferStatusExt returned an error." << endl;
  return false;
}


/**
 * Draw the background (the image from the camera)
 */
void ARDriver::DrawFBBackGround()
{
  static bool bFirstRun = true;
  static GLuint nList;
  mGLWindow.SetupUnitOrtho();

  glEnable(GL_TEXTURE_RECTANGLE_ARB);
  glBindTexture(GL_TEXTURE_RECTANGLE_ARB, mnFrameTex);
  glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glDisable(GL_POLYGON_SMOOTH);
  glDisable(GL_BLEND);
  // Cache the cpu-intesive projections in a display list..
  if(bFirstRun)
    {
      bFirstRun = false;
      nList = glGenLists(1);
      glNewList(nList, GL_COMPILE_AND_EXECUTE);
      glColor3f(1,1,1);
      // How many grid divisions in the x and y directions to use?
      int nStepsX = 24; // Pretty arbitrary..
      int nStepsY = (int) (nStepsX * ((double) mirFrameSize.x / mirFrameSize.y)); // Scaled by aspect ratio
      if(nStepsY < 2)
	nStepsY = 2;
      for(int ystep = 0; ystep< nStepsY; ystep++)
	{
	  glBegin(GL_QUAD_STRIP);
	  for(int xstep = 0; xstep <= nStepsX; xstep++)
	    for(int yystep = ystep; yystep<=ystep+1; yystep++) // Two y-coords in one go - magic.
	      {
		Vector<2> v2Iter;
		v2Iter[0] = (double) xstep / nStepsX;
		v2Iter[1] = (double) yystep / nStepsY;
		// If this is a border quad, draw a little beyond the
		// outside of the frame, this avoids strange jaggies
		// at the edge of the reconstructed frame later:
		if(xstep == 0 || yystep == 0 || xstep == nStepsX || yystep == nStepsY)
		  for(int i=0; i<2; i++)
		    v2Iter[i] = v2Iter[i] * 1.02 - 0.01;
		Vector<2> v2UFBDistorted = v2Iter;
		Vector<2> v2UFBUnDistorted = mCamera.UFBLinearProject(mCamera.UFBUnProject(v2UFBDistorted));
		glTexCoord2d(v2UFBDistorted[0] * mirFrameSize.x, v2UFBDistorted[1] * mirFrameSize.y);
		glVertex(v2UFBUnDistorted);
	      }
	  glEnd();
	}
      glEndList();
    }
  else
    glCallList(nList);
  glDisable(GL_TEXTURE_RECTANGLE_ARB);
}


/**
 * Draw the distorted frame buffer
 */
void ARDriver::DrawDistortedFB()
{
  static bool bFirstRun = true;
  static GLuint nList;
  mGLWindow.SetupViewport();
  mGLWindow.SetupUnitOrtho();
  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();
  glEnable(GL_TEXTURE_RECTANGLE_ARB);
  glBindTexture(GL_TEXTURE_RECTANGLE_ARB, mnFrameBufferTex);
  glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glDisable(GL_POLYGON_SMOOTH);
  glDisable(GL_BLEND);
  if(bFirstRun)
    {
      bFirstRun = false;
      nList = glGenLists(1);
      glNewList(nList, GL_COMPILE_AND_EXECUTE);
      // How many grid divisions in the x and y directions to use?
      int nStepsX = 24; // Pretty arbitrary..
      int nStepsY = (int) (nStepsX * ((double) mirFrameSize.x / mirFrameSize.y)); // Scaled by aspect ratio
      if(nStepsY < 2)
	nStepsY = 2;
      glColor3f(1,1,1);
      for(int ystep = 0; ystep<nStepsY; ystep++)
	{
	  glBegin(GL_QUAD_STRIP);
	  for(int xstep = 0; xstep<=nStepsX; xstep++)
	    for(int yystep = ystep; yystep<=ystep + 1; yystep++) // Two y-coords in one go - magic.
	      {
		Vector<2> v2Iter;
		v2Iter[0] = (double) xstep / nStepsX;
		v2Iter[1] = (double) yystep / nStepsY;
		Vector<2> v2UFBDistorted = v2Iter;
		Vector<2> v2UFBUnDistorted = mCamera.UFBLinearProject(mCamera.UFBUnProject(v2UFBDistorted));
		glTexCoord2d(v2UFBUnDistorted[0] * mirFBSize.x, (1.0 - v2UFBUnDistorted[1]) * mirFBSize.y);
		glVertex(v2UFBDistorted);
	      }
	  glEnd();
	}
      glEndList();
    }
  else
    glCallList(nList);
  glDisable(GL_TEXTURE_RECTANGLE_ARB);
}

/**
 * Draw the fading grid
 */
void ARDriver::DrawFadingGrid()
{
  double dStrength;
  if(mnCounter >= 60)
    return;
  if(mnCounter < 30)
    dStrength = 1.0;
  dStrength = (60 - mnCounter) / 30.0;

  glColor4f(1,1,1,dStrength);
  int nHalfCells = 8;
  if(mnCounter < 8)
    nHalfCells = mnCounter + 1;
  int nTot = nHalfCells * 2 + 1;
  Vector<3>  aaVertex[17][17];
  for(int i=0; i<nTot; i++)
    for(int j=0; j<nTot; j++)
      {
	Vector<3> v3;
	v3[0] = (i - nHalfCells) * 0.1;
	v3[1] = (j - nHalfCells) * 0.1;
	v3[2] = 0.0;
	aaVertex[i][j] = v3;
      }

  glEnable(GL_LINE_SMOOTH);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glLineWidth(2);
  for(int i=0; i<nTot; i++)
    {
      glBegin(GL_LINE_STRIP);
      for(int j=0; j<nTot; j++)
	glVertex(aaVertex[i][j]);
      glEnd();

      glBegin(GL_LINE_STRIP);
      for(int j=0; j<nTot; j++)
	glVertex(aaVertex[j][i]);
      glEnd();
    };
}


/**
 * What to do when the user clicks on the screen.
 * Calculates the 3d postion of the click on the plane
 * and passes info to a game, if there is one.
 * @param nButton the button pressed
 * @param irWin the window x, y location 
 */
void ARDriver::HandleClick(int nButton, ImageRef irWin )
{
  //The window may have been resized, so want to work out the coords based on the orignal image size
  Vector<2> v2VidCoords = mGLWindow.VidFromWinCoords( irWin );
  
  
  Vector<2> v2UFBCoords;
#ifdef WIN32
  Vector<2> v2PlaneCoords;   v2PlaneCoords[0] = numeric_limits<double>::quiet_NaN();   v2PlaneCoords[1] = numeric_limits<double>::quiet_NaN();
#else
  Vector<2> v2PlaneCoords;   v2PlaneCoords[0] = NAN;   v2PlaneCoords[1] = NAN;
#endif
  Vector<3> v3RayDirn_W;

  // Work out image coords 0..1:
  v2UFBCoords[0] = (v2VidCoords[0] + 0.5) / mCamera.GetImageSize()[0];
  v2UFBCoords[1] = (v2VidCoords[1] + 0.5) / mCamera.GetImageSize()[1];

  // Work out plane coords:
  Vector<2> v2ImPlane = mCamera.UnProject(v2VidCoords);
  Vector<3> v3C = unproject(v2ImPlane);
  Vector<4> v4C = unproject(v3C);
  SE3<> se3CamInv = mse3CfromW.inverse();
  Vector<4> v4W = se3CamInv * v4C;
  double t = se3CamInv.get_translation()[2];
  double dDistToPlane = -t / (v4W[2] - t);

  if(v4W[2] -t <= 0) // Clicked the wrong side of the horizon?
  {
    v4C.slice<0,3>() *= dDistToPlane;
    Vector<4> v4Result = se3CamInv * v4C;
    v2PlaneCoords = v4Result.slice<0,2>(); // <--- result
  }

  // Ray dirn:
  v3RayDirn_W = v4W.slice<0,3>() - se3CamInv.get_translation();
  normalize(v3RayDirn_W);

  if(mpMap->pGame) {
    mpMap->pGame->HandleClick(v2VidCoords, v2UFBCoords, v3RayDirn_W, v2PlaneCoords, nButton);
  }
}



/**
 * Handle the user pressing a key
 * @param sKey the key the user pressed.
 */

void ARDriver::HandleKeyPress( std::string sKey )
{
  if(mpMap && mpMap->pGame ) {
    mpMap->pGame->HandleKeyPress( sKey );
  }
/*

    if( sKey == "Enter")
    {
          if(as.get_state()!=STATE_INIT)
            printf("interdit");
          else
            as.set_state(STATE_START);
          std::cout<<buffer<<std::endl;
    }
    else if( sKey == "Space" )
    {
        buffer+=" ";
    }
    else if( sKey == "BackSpace" )
    {
        buffer=buffer.substr(0,buffer.size()-1);
    }
    else if(sKey.size()==1
    &&strchr("1234567890QWWERTYUIOPASDFGHJKLZXCVBNMqwertyuioipasdfghjklzxcvbnm!?.,",(sKey.c_str())[0]))
    {
        buffer+=sKey;//age[strlen(message)]=(sKey.c_str())[0];
        std::cout<<sKey<<std::endl; 
   }else if( sKey == "Gauche" )
    {
        thetaY+=10;
    }
    else if( sKey == "Droit" )
    {
        thetaY-=10;
    }
    else if( sKey == "Haut" )
    {
        thetaX+=10;
    }
    else if( sKey == "Bas" )
    {
        thetaX-=10;
    }
    else if( sKey == "Petit" )
    {
        scale/=1.3;
    }
    else if( sKey == "Grand" )
    {
        scale*=1.3;
    }

*/


}


/**
 * Load a game by name.
 * @param sName Name of the game
 */
void ARDriver::LoadGame(std::string sName)
{
  if(mpMap->pGame)
  {
    delete mpMap->pGame;
    mpMap->pGame = NULL;
  }

  mpMap->pGame = LoadAGame( sName, "");
  if( mpMap->pGame ) {
    mpMap->pGame->Init();
  }
 
}



/**
 * Advance the game logic
 */
void ARDriver::AdvanceLogic()
{
  if(mpMap->pGame) {
    mpMap->pGame->Advance();
  }
}


}
