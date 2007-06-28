
#include <QApplication>
#include "QTestApp.h"

#include "pqAnimationWidget.h"
#include "pqAnimationModel.h"
#include "pqAnimationTrack.h"
#include "pqAnimationKeyFrame.h"

int Animation(int argc, char* argv[])
{
  QTestApp app(argc, argv);

  pqAnimationWidget view;
  view.resize(400,300);

  pqAnimationModel* scene = view.animationModel();

  scene->setStartTime(0.0);
  scene->setEndTime(1.5);
  scene->setCurrentTime(5.0);

  pqAnimationTrack* track;
  pqAnimationKeyFrame* keyFrame;
 
  track = scene->addTrack();
  track->setProperty("dummy1");
  keyFrame = track->addKeyFrame();
  keyFrame->setStartTime(0);
  keyFrame->setEndTime(1);
  keyFrame->setStartValue(0.0);
  keyFrame->setEndValue(1.0);
  
  track = scene->addTrack();
  track->setProperty("dummy");
  keyFrame = track->addKeyFrame();
  keyFrame->setStartTime(0.75);
  keyFrame->setEndTime(1);
  keyFrame->setStartValue(25);
  keyFrame->setEndValue(25);
  
  track = scene->addTrack();
  keyFrame = track->addKeyFrame();
  keyFrame->setStartTime(0);
  keyFrame->setEndTime(0.5);
  keyFrame->setStartValue(0);
  keyFrame->setEndValue(360);
  
  track = scene->addTrack();
  keyFrame = track->addKeyFrame();
  keyFrame->setStartTime(0.5);
  keyFrame->setEndTime(1.0);
  keyFrame->setStartValue(0);
  keyFrame->setEndValue(360);
  
  track = scene->addTrack();
  keyFrame = track->addKeyFrame();
  keyFrame->setStartTime(0.25);
  keyFrame->setEndTime(0.6);
  keyFrame->setStartValue(0);
  keyFrame->setEndValue(1.0);
  
  keyFrame = track->addKeyFrame();
  keyFrame->setStartTime(0.6);
  keyFrame->setEndTime(0.9);
  keyFrame->setStartValue(1.0);
  keyFrame->setEndValue(0);

  view.show();

  return app.exec();
}

