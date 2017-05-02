# DO NOT DELETE THIS LINE -- make depend depends on it.


#3DS modelling game dependencies
3DSLIB = -l3ds
3DSFILES =	ModelsGame.o\
		ModelBrowser.o\
		ModelControls.o\
		MGButton.o\
		Model3ds.o\
		ModelsGameData.o

EXTRA_CFLAGS:= -DENABLE_MODELS_GAME

# Edit the lines below to point to any needed include and link paths
# Or to change the compiler's optimization flags
FBXLINKER = -lfbxsdk -lm -lrt -luuid -lstdc++ -lpthread -ldl -lGLU -lGL -lglut -lGLEW -lXmu
CC = g++
COMPILEFLAGS = -I -I/usr/local/include -D_LINUX -D_REENTRANT -Wall  -O3 -march=nocona -msse3 -fno-strict-aliasing -I./fbx/include
LINKFLAGS = -L  MY_CUSTOM_LINK_PATH -lGVars3 -lcvd $(3DSLIB) -lGL -lGLU -L/usr/local/lib -llapack -lblas `pkg-config --libs opencv` -L./fbx/lib/gcc4/x64/release $(FBXLINKER) 

# Edit this line to change video source
VIDEOSOURCE = VideoSource_Linux_OpenCV.o


FBXDIR = fbx/
FBXCODE = $(FBXDIR)Common.o\
        $(FBXDIR)DrawScene.o\
	    $(FBXDIR)DrawText.o\
	    $(FBXDIR)GetPosition.o\
	    $(FBXDIR)SceneCache.o\
	    $(FBXDIR)SceneContext.o\
	    $(FBXDIR)GlFunctions.o\
	    $(FBXDIR)SetCamera.o\
	    $(FBXDIR)targa.o\

OBJECTS=	$(FBXCODE)\
		main.o\
		GLWindow2.o\
		GLWindowMenu.o\
		System.o \
		ATANCamera.o\
		KeyFrame.o\
		MapPoint.o\
		Map.o\
		SmallBlurryImage.o\
		ShiTomasi.o \
		HomographyInit.o \
		MapMaker.o \
		Bundle.o \
		PatchFinder.o\
		Relocaliser.o\
		MiniPatch.o\
		MapViewer.o\
		ARDriver.o\
		EyeGame.o\
		Tracker.o\
		tinyxml.o\
		tinyxmlerror.o\
		tinyxmlparser.o\
		MapLockManager.o\
		MD5.o\
		MD5Wrapper.o\
		MapSerializer.o\
		Games.o\
		Utils.o\
		ShooterGame.o\
		ShooterGameTarget.o\
		$(VIDEOSOURCE)\
        AliceScript.o\
		$(3DSFILES)


CALIB_OBJECTS=	GLWindow2.o\
		GLWindowMenu.o\
		$(VIDEOSOURCE)\
		CalibImage.o \
		CalibCornerPatch.o\
		ATANCamera.o \
		CameraCalibrator.o

All: PTAMM CameraCalibrator

PTAMM: $(OBJECTS)
	$(CC) -o PTAMM $(OBJECTS) $(LINKFLAGS)

CameraCalibrator:$(CALIB_OBJECTS)
	$(CC) -o CameraCalibrator $(CALIB_OBJECTS) $(LINKFLAGS)


%.o: %.cc
	$(CC) $< -o $@ -c $(COMPILEFLAGS) $(EXTRA_CFLAGS)

clean:
	rm *.o


depend:
	rm dependecies; touch dependencies
	makedepend -fdependencies $(INCLUDEFLAGS) $(MOREINCS) *.cc *.h


-include dependencies









