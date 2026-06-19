#include <iostream>
#include <algorithm> 
#include <random>   
#include <iterator>
#include <cmath>
#include <QApplication>
#include <Qt3DExtras/Qt3DWindow>
#include <Qt3DCore/QEntity>
#include <Qt3DRender/QCamera>
#include <Qt3DExtras/QOrbitCameraController>
#include <Qt3DExtras/QCylinderMesh>
#include <Qt3DExtras/QCuboidMesh>
#include <Qt3DExtras/QPhongMaterial>
#include <Qt3DCore/QTransform>
#include <Qt3DRender/QDirectionalLight>
#include <Qt3DRender/QPointLight>
#include <Qt3DRender/QObjectPicker>
#include <Qt3DRender/QPickEvent>
#include <QVector>
#include <QRandomGenerator>
#include <QTimer>
#include <QLabel>
#include <QPropertyAnimation>
#include <Qt3DExtras/QExtrudedTextMesh>
#include <QKeyEvent>
#include <QShortcut>
#include <QSoundEffect>
#include <QMediaPlayer>
#include <QAudioOutput>

using namespace std;

float dice_size = 0.6f;
float table_height = 1.0f;
float table_size = 3.0f;
int num_of_dice = 0;

struct FallingDice{
  Qt3DCore::QEntity *entity;
  Qt3DCore::QTransform *transform;
  float velocityY;
  float velocityX;
  float velocityZ;
  float positionX;
  float positionY;
  float positionZ;
  float rotationX;
  float rotationY;
  float rotationZ;
  float rotSpeedX;
  float rotSpeedY;
  float rotSpeedZ;
  bool isFalling;
};

int main(int argc, char *argv[]){
    QApplication app(argc, argv);

    QMediaPlayer *roll_sound = new QMediaPlayer();
    QAudioOutput *roll_output = new QAudioOutput();

    QMediaPlayer *reset_sound = new QMediaPlayer();
    QAudioOutput *reset_output = new QAudioOutput();
    
    Qt3DExtras::Qt3DWindow view;
    
    Qt3DCore::QEntity *rootEntity = new Qt3DCore::QEntity();
    
    Qt3DRender::QCamera *camera = view.camera();
    camera->setPosition(QVector3D(0, 12, 8));  
    camera->setViewCenter(QVector3D(0, 1.5f, 0)); // Look at center of room
    camera->setUpVector(QVector3D(0, 1, 0));  

    Qt3DExtras::QOrbitCameraController *controller = new Qt3DExtras::QOrbitCameraController(rootEntity);
    controller->setCamera(camera);
    
    // Lighting
    Qt3DRender::QDirectionalLight *light1 = new Qt3DRender::QDirectionalLight(rootEntity);
    light1->setColor(QColor(255,255,255));
    light1->setIntensity(5.0f);
    light1->setWorldDirection(QVector3D(0.8f, -1.0f, -0.5f));
    
    Qt3DRender::QDirectionalLight *light2 = new Qt3DRender::QDirectionalLight(rootEntity);
    light2->setColor(QColor(255,255,255));
    light2->setIntensity(5.0f);
    light2->setWorldDirection(QVector3D(0.5f, -0.8f, 0.5f));

    Qt3DRender::QDirectionalLight *rimLight = new Qt3DRender::QDirectionalLight(rootEntity);
    rimLight->setColor(QColor(200, 200, 255));
    rimLight->setIntensity(0.5f);
    rimLight->setWorldDirection(QVector3D(0, -0.5f, 1.0f));
 
    // Table
    Qt3DExtras::QCylinderMesh *tableTopMesh = new Qt3DExtras::QCylinderMesh();
    tableTopMesh->setRadius(3.0f);
    tableTopMesh->setLength(0.2f);
    tableTopMesh->setRings(50);
    tableTopMesh->setSlices(50);
    
    Qt3DExtras::QPhongMaterial *tableTopMaterial = new Qt3DExtras::QPhongMaterial();
    tableTopMaterial->setDiffuse(QColor(30, 100, 30));
    tableTopMaterial->setAmbient(QColor(80, 20, 20));
    tableTopMaterial->setSpecular(Qt::white);
    tableTopMaterial->setShininess(100);
    
    Qt3DCore::QTransform *tableTopTransform = new Qt3DCore::QTransform();
    tableTopTransform->setTranslation(QVector3D(0, 1.2f, 0));
    
    Qt3DCore::QEntity *tableTop = new Qt3DCore::QEntity(rootEntity);
    tableTop->addComponent(tableTopMesh);
    tableTop->addComponent(tableTopMaterial);
    tableTop->addComponent(tableTopTransform);
    
    // Table picker
    Qt3DRender::QObjectPicker *tablePicker = new Qt3DRender::QObjectPicker();
    tablePicker->setDragEnabled(true);
    tableTop->addComponent(tablePicker);
    
    // Vector to store falling boxes
    QVector<FallingDice> fallingDices;
 
    auto addTextToFace = [&](Qt3DCore::QEntity *parent, const QString &text, const QVector3D &rotation) {
    Qt3DExtras::QExtrudedTextMesh *textMesh = new Qt3DExtras::QExtrudedTextMesh();
    textMesh->setText(text);
    textMesh->setDepth(0.04f); // Keeps text thin and crisp
    
    Qt3DExtras::QPhongMaterial *textMaterial = new Qt3DExtras::QPhongMaterial();
    textMaterial->setDiffuse(Qt::black);
    
    Qt3DCore::QTransform *textTransform = new Qt3DCore::QTransform();
    
    // 1. Center the text alignment bounding box at scale 1.0f
    // (A single digit is roughly 0.15 wide and 0.20 high)
    float fontWidthOffset = -0.075f;
    float fontHeightOffset = -0.10f;
    
    // 2. Build the transformations sequentially using a Matrix
    QMatrix4x4 localMatrix;
    
    // First: Apply the face rotation
    localMatrix.rotate(QQuaternion::fromEulerAngles(rotation));
    
    // Second: Push the text outward to the face skin along the face's local Z-axis
    // We add a tiny 0.005f gap to avoid z-fighting / flickering textures
    float outwardOffset = (dice_size / 2.0f) + 0.005f;
    localMatrix.translate(QVector3D(fontWidthOffset, fontHeightOffset, outwardOffset));
    
    // 3. Extract the final translated vector and apply it
    textTransform->setTranslation(localMatrix.map(QVector3D(0, 0, 0)));
    textTransform->setRotation(QQuaternion::fromEulerAngles(rotation));
    textTransform->setScale(0.4f); // Applies your custom scale factor
    
    Qt3DCore::QEntity *textEntity = new Qt3DCore::QEntity(parent);
    textEntity->addComponent(textMesh);
    textEntity->addComponent(textMaterial);
    textEntity->addComponent(textTransform);
};

    // Create a dedicated counter display 
    Qt3DCore::QEntity *counterEntity = new Qt3DCore::QEntity(rootEntity);
    Qt3DCore::QTransform *counterTransform = new Qt3DCore::QTransform();
    counterTransform->setTranslation(QVector3D(-2.5f, 9.2f, -1.2f));
    counterTransform->setScale3D(QVector3D(0.5f,0.5f,0.5f));

    counterEntity->addComponent(counterTransform);
    
    // Create a dedicated text mesh for the counter
    Qt3DExtras::QExtrudedTextMesh *counterTextMesh = new Qt3DExtras::QExtrudedTextMesh();
    counterTextMesh->setText(QString("Number of dice: 0"));
    counterTextMesh->setDepth(0.04f);
    
    Qt3DExtras::QPhongMaterial *counterMaterial = new Qt3DExtras::QPhongMaterial();
    counterMaterial->setDiffuse(Qt::black);

    counterEntity->addComponent(counterTextMesh);
    counterEntity->addComponent(counterMaterial);
    
    // Store pointer globally or capture in lambda
    // Then update it directly:
    counterTextMesh->setText(QString("Number of dice: %1").arg(num_of_dice));
       
// Physics timer (60 FPS = ~16ms)
QTimer *physicsTimer = new QTimer();
physicsTimer->setInterval(16);
QObject::connect(physicsTimer, &QTimer::timeout, [&](){
    const float GRAVITY = -9.8f;
    const float GROUND_Y = 1.7f;
    const float TIME_STEP = 1.0f / 60.0f;

    for(int i = 0; i < fallingDices.size(); i++){
        FallingDice &die = fallingDices[i];
        
        if(die.isFalling){
            // Apply gravity and movement
            die.velocityY += GRAVITY * TIME_STEP;
            die.positionY += die.velocityY * TIME_STEP;
            die.positionX += die.velocityX * TIME_STEP;
            die.positionZ += die.velocityZ * TIME_STEP;
            die.rotationX += die.rotSpeedX * TIME_STEP;
            die.rotationY += die.rotSpeedY * TIME_STEP;
            die.rotationZ += die.rotSpeedZ * TIME_STEP;

            // Check if landed on table
            if(die.positionY <= GROUND_Y){
                die.positionY = GROUND_Y;
                die.velocityY = 0;
                die.rotationX = round(die.rotationX / 90.0f) * 90.0f;
                die.rotationY = round(die.rotationY / 90.0f) * 90.0f;
                die.rotationZ = round(die.rotationZ / 90.0f) * 90.0f;
                die.isFalling = false;
            }

            // Apply transforms
            die.transform->setTranslation(QVector3D(die.positionX, die.positionY, die.positionZ));
            die.transform->setRotation(QQuaternion::fromEulerAngles(die.rotationX, die.rotationY, die.rotationZ));
        }
    }

    // Remove dice that rolled off the table (after physics update)
    for(int i = 0; i < fallingDices.size(); i++){
        FallingDice &die = fallingDices[i];
        float distFromCenter = sqrt(pow(die.positionX, 2) + pow(die.positionZ, 2));
        
        if(!die.isFalling && distFromCenter >= table_size){
            die.entity->deleteLater();
            fallingDices.remove(i);
            num_of_dice--;
            counterTextMesh->setText(QString("Number of dice: %1").arg(num_of_dice));
            i--;
        }
    }

        // 2. DICE-TO-DICE COLLISION CHECKING
        for(int i = 0; i < fallingDices.size(); i++){
            for(int j = i + 1; j < fallingDices.size(); j++){
                
	    FallingDice &die_i = fallingDices[i];
	    FallingDice &die_j = fallingDices[j];
	    float distance_squared = pow((die_i.positionX - die_j.positionX),2) + pow((die_i.positionY - die_j.positionY),2) + pow((die_i.positionZ - die_j.positionZ),2);
	    
	 if(die_i.entity != die_j.entity){
    // 1. Calculate relative distance vectors
    QVector3D posA(die_i.positionX, die_i.positionY, die_i.positionZ);
    QVector3D posB(die_j.positionX, die_j.positionY, die_j.positionZ);
    QVector3D delta = posA - posB;
    float distance = delta.length();
    
    // Using a bounding sphere approximation for dice size
    float collisionDistance = 2.0f * dice_size; 

    if(distance <= collisionDistance && distance > 0.001f){
        // Collision Normal (pointing from J to I)
        QVector3D normal = delta / distance;
        float overlap = collisionDistance - distance;

        // ==========================================
        // PHASE 1: EQUAL POSITIONAL CORRECTION (Anti-Sinking)
        // ==========================================
        // Push BOTH dice away from each other equally (0.5f each)
        float correctionAmount = overlap * 0.5f;
        
        die_i.positionX += normal.x() * correctionAmount;
        die_i.positionZ += normal.z() * correctionAmount;

        die_j.positionX -= normal.x() * correctionAmount;
        die_j.positionZ -= normal.z() * correctionAmount;

        // Apply corrected positions to Qt transforms immediately
        die_i.transform->setTranslation(QVector3D(die_i.positionX, die_i.positionY, die_i.positionZ));
        die_j.transform->setTranslation(QVector3D(die_j.positionX, die_j.positionY, die_j.positionZ));

        // ==========================================
        // PHASE 2: IMPULSE VELOCITY RESPONSE
        // ==========================================
        // Assumes your die struct/class has velocity variables (e.g., die_i.vx, die_i.vy, die_i.vz)
        QVector3D velA(die_i.velocityX, die_i.velocityY, die_i.velocityZ);
        QVector3D velB(die_j.velocityX, die_j.velocityY, die_j.velocityZ);
        
        // Relative velocity along the normal vector
        QVector3D relVel = velA - velB;
        float velAlongNormal = QVector3D::dotProduct(relVel, normal);

        // Only resolve if they are moving TOWARDS each other
        if(velAlongNormal < 0){
            // bounciness factor (0.0 = no bounce, 1.0 = perfect bounce)
            float restitution = 1.0f; 
            
            // Calculate scalar impulse (assuming equal mass for both dice)
            float impulseScalar = -(1.0f + restitution) * velAlongNormal;
            impulseScalar /= 2.0f; // Divided by (1/massA + 1/massB). If mass is 1, this is 1+1 = 2.

            // Apply impulse to change velocities
            QVector3D impulse = impulseScalar * normal;
            
            die_i.velocityX += impulse.x();
            die_i.velocityY += impulse.y();
            die_i.velocityZ += impulse.z();

            die_j.velocityX -= impulse.x();
            die_j.velocityY -= impulse.y();
            die_j.velocityZ -= impulse.z();
        }
    }
}
	  }
	  }
    });

    physicsTimer->start(); 
    
    // Click to create falling die
        QObject::connect(tablePicker, &Qt3DRender::QObjectPicker::clicked, 
        [&](Qt3DRender::QPickEvent *event){
	  num_of_dice++;
	  counterTextMesh->setText(QString("Number of dice: %1").arg(num_of_dice));
	  QVector3D clickPos = event->worldIntersection();
	  float tableSurfaceY = 1.5;
	  float StartX = clickPos.x();
	  float StartY = clickPos.y();
	  float StartZ = clickPos.z();

            // Create the dice cube
	  Qt3DExtras::QCuboidMesh *boxMesh = new Qt3DExtras::QCuboidMesh();
	  boxMesh->setXExtent(dice_size);
	  boxMesh->setYExtent(dice_size);
	  boxMesh->setZExtent(dice_size);

	  Qt3DExtras::QPhongMaterial *diceMaterial = new Qt3DExtras::QPhongMaterial();
	    
            // Random dice value (1-6)
	  int diceValue[] = {1,2,3,4,5,6};

	  random_device rd;
	  mt19937 g(rd());
	  shuffle(begin(diceValue),end(diceValue),g);
	    
            // Create the dice entity
	  Qt3DCore::QEntity *dice = new Qt3DCore::QEntity(rootEntity);
	  dice->addComponent(boxMesh);
	  dice->addComponent(diceMaterial);
            
            // Add transform
	  Qt3DCore::QTransform *diceTransform = new Qt3DCore::QTransform();

	  diceMaterial->setAmbient(QColor(100,100,100));  // Base color in shadow
	  diceMaterial->setDiffuse(QColor(255, 255, 255));  // Base color under direct light
	  diceMaterial->setSpecular(Qt::white);             // Highlighting color
	  diceMaterial->setShininess(50.0f);
    
	  diceTransform->setTranslation(QVector3D(StartX, StartY, StartZ));
	  dice->addComponent(diceTransform);
	    
	    // Add text to all 6 faces
	  addTextToFace(dice, QString::number(diceValue[0]), QVector3D(0, 0, 0));       // Front
	  addTextToFace(dice, QString::number(diceValue[1]), QVector3D(0, 180, 0));     // Back
	  addTextToFace(dice, QString::number(diceValue[2]), QVector3D(-90, 0, 0));     // Top
	  addTextToFace(dice, QString::number(diceValue[3]), QVector3D(90, 0, 0));      // Bottom
	  addTextToFace(dice, QString::number(diceValue[4]), QVector3D(0, -90, 0));     // Right
	  addTextToFace(dice, QString::number(diceValue[5]), QVector3D(0, 90, 0));      // Left

        
        // Add to physics simulation
	  FallingDice newDice;
	  newDice.entity = dice;
	  newDice.transform = diceTransform;
	  newDice.velocityX = (float)(rand() % 400 - 200) / 100.0f;
	  newDice.velocityZ = (float)(rand() % 400 - 200) / 100.0f;
	  newDice.velocityY = 0.0f;
	  newDice.positionX = clickPos.x();
	  newDice.positionY = StartY;
	  newDice.positionZ = clickPos.z();
	  newDice.rotationX = rand() % 360;
	  newDice.rotationY = rand() % 360;
	  newDice.rotationZ = rand() % 360;
	  newDice.rotSpeedX = (rand() % 300) + 200;
	  newDice.rotSpeedY = (rand() % 300) + 200;
	  newDice.rotSpeedZ = (rand() % 300) + 200;
	  newDice.isFalling = true;
	  fallingDices.append(newDice);
	  
	  roll_sound->setAudioOutput(roll_output);
	  roll_sound->setSource(QUrl::fromLocalFile("dice_roll.mp3"));
	  roll_sound->play();
	  
	});
	
    // Table legs
	Qt3DExtras::QCylinderMesh *legMesh = new Qt3DExtras::QCylinderMesh();
	legMesh->setRadius(0.2f);
	legMesh->setLength(1.2f);
	legMesh->setRings(20);
	legMesh->setSlices(20);
    
	Qt3DExtras::QPhongMaterial *legMaterial = new Qt3DExtras::QPhongMaterial();
	legMaterial->setDiffuse(QColor(101, 67, 33));    
	legMaterial->setAmbient(QColor(50, 33, 16));
	legMaterial->setSpecular(Qt::lightGray);
	legMaterial->setShininess(60);
	
	QVector3D legPositions[] = {
	  QVector3D(-2.2f, 0.6f, -2.2f),
	  QVector3D( 2.2f, 0.6f, -2.2f),
	  QVector3D(-2.2f, 0.6f,  2.2f),
	  QVector3D( 2.2f, 0.6f,  2.2f)
	};
	
	for(const auto &pos : legPositions){
	  Qt3DCore::QTransform *legTransform = new Qt3DCore::QTransform();
	  legTransform->setTranslation(pos);
	  
	  Qt3DCore::QEntity *leg = new Qt3DCore::QEntity(rootEntity);
	  leg->addComponent(legMesh);
	  leg->addComponent(legMaterial);
	  leg->addComponent(legTransform);
	}
	
	// Table rim
	Qt3DExtras::QCylinderMesh *rimMesh = new Qt3DExtras::QCylinderMesh();
	rimMesh->setRadius(3.05f);
	rimMesh->setLength(0.1f);
	rimMesh->setRings(50);
	rimMesh->setSlices(50);
    
	Qt3DExtras::QPhongMaterial *rimMaterial = new Qt3DExtras::QPhongMaterial();
	rimMaterial->setDiffuse(QColor(30,100,30)); 
	
	Qt3DCore::QTransform *rimTransform = new Qt3DCore::QTransform();
	rimTransform->setTranslation(QVector3D(0, 1.3f, 0));
	
	Qt3DCore::QEntity *tableRim = new Qt3DCore::QEntity(rootEntity);
	tableRim->addComponent(rimMesh);
	tableRim->addComponent(rimMaterial);
	tableRim->addComponent(rimTransform);
	
	// Ground
	Qt3DExtras::QCuboidMesh *groundMesh = new Qt3DExtras::QCuboidMesh();
	groundMesh->setXExtent(15.0f);
	groundMesh->setYExtent(0.1f);
	groundMesh->setZExtent(15.0f);
	
	Qt3DExtras::QPhongMaterial *groundMaterial = new Qt3DExtras::QPhongMaterial();
	groundMaterial->setDiffuse(QColor(50,50,60));
	groundMaterial->setShininess(150);
	groundMaterial->setSpecular(Qt::white);
    
	Qt3DCore::QTransform *groundTransform = new Qt3DCore::QTransform();
	groundTransform->setTranslation(QVector3D(0, 0, 0));
    
	Qt3DCore::QEntity *ground = new Qt3DCore::QEntity(rootEntity);
	ground->addComponent(groundMesh);
	ground->addComponent(groundMaterial);
	ground->addComponent(groundTransform);
	
	// Background
	Qt3DExtras::QCuboidMesh *bgMesh = new Qt3DExtras::QCuboidMesh();
	bgMesh->setXExtent(20.0f);
	bgMesh->setYExtent(20.0f);
	bgMesh->setZExtent(0.1f);
	
	Qt3DExtras::QPhongMaterial *bgMaterial = new Qt3DExtras::QPhongMaterial();
	bgMaterial->setDiffuse(QColor(0,102,0));

	// Wall dimensions
	float wallWidth = 10.0f;   // Width of each wall
	float wallHeight = 4.0f;   // Height of wall
	float wallThick = 0.2f;    // Thickness of wall

	// Create materials for walls
	Qt3DExtras::QPhongMaterial *wallMaterial = new Qt3DExtras::QPhongMaterial();
	wallMaterial->setDiffuse(QColor(255, 0, 0));   // Wood color walls
	wallMaterial->setAmbient(QColor(60, 50, 40));

	// Create mesh for front/back walls (wide horizontally)
	Qt3DExtras::QCuboidMesh *frontBackMesh = new Qt3DExtras::QCuboidMesh();
	frontBackMesh->setXExtent(wallWidth);
	frontBackMesh->setYExtent(wallHeight);
	frontBackMesh->setZExtent(wallThick);

	// Create mesh for side walls (wide vertically)
	Qt3DExtras::QCuboidMesh *sideMesh = new Qt3DExtras::QCuboidMesh();
	sideMesh->setXExtent(wallThick);
	sideMesh->setYExtent(wallHeight);
	sideMesh->setZExtent(wallWidth);

	float wallDistance = 5.0f;  // How far walls are from center

	// === BACK WALL (Z negative) ===
	Qt3DCore::QTransform *backWallTransform = new Qt3DCore::QTransform();
	backWallTransform->setTranslation(QVector3D(0, 1.5f, -wallDistance));
	Qt3DCore::QEntity *backWall = new Qt3DCore::QEntity(rootEntity);
	backWall->addComponent(frontBackMesh);
	backWall->addComponent(wallMaterial);
	backWall->addComponent(backWallTransform);

	// === FRONT WALL (Z positive) ===
	Qt3DCore::QTransform *frontWallTransform = new Qt3DCore::QTransform();
	frontWallTransform->setTranslation(QVector3D(0, 1.5f, wallDistance));
	Qt3DCore::QEntity *frontWall = new Qt3DCore::QEntity(rootEntity);
	frontWall->addComponent(frontBackMesh);
	frontWall->addComponent(wallMaterial);
	frontWall->addComponent(frontWallTransform);

	// === LEFT WALL (X negative) ===
	Qt3DCore::QTransform *leftWallTransform = new Qt3DCore::QTransform();
	leftWallTransform->setTranslation(QVector3D(-wallDistance, 1.5f, 0));
	Qt3DCore::QEntity *leftWall = new Qt3DCore::QEntity(rootEntity);
	leftWall->addComponent(sideMesh);
	leftWall->addComponent(wallMaterial);
	leftWall->addComponent(leftWallTransform);
	
	// === RIGHT WALL (X positive) ===
	Qt3DCore::QTransform *rightWallTransform = new Qt3DCore::QTransform();
	rightWallTransform->setTranslation(QVector3D(wallDistance, 1.5f, 0));
	Qt3DCore::QEntity *rightWall = new Qt3DCore::QEntity(rootEntity);
	rightWall->addComponent(sideMesh);
	rightWall->addComponent(wallMaterial);
	rightWall->addComponent(rightWallTransform); 
 
	class KeyPressFilter : public QObject{
	public:
	  KeyPressFilter(QVector<FallingDice> &dice, Qt3DRender::QCamera *cam, Qt3DCore::QEntity *root, QMediaPlayer *resetSound, QAudioOutput *resetOutput, Qt3DExtras::QExtrudedTextMesh *counterText) 
	  : m_dice(dice), m_camera(cam), m_root(root), m_resetSound(resetSound), m_resetOutput(resetOutput), m_counterText(counterText) {}
    
	  bool eventFilter(QObject *obj, QEvent *event) override{
	    if(event->type() == QEvent::KeyPress){
	      QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
            
	      // Camera rotation with arrow keys
	      if(keyEvent->key() == Qt::Key_Left){
                QVector3D pos = m_camera->position();
                // Rotate around Y axis (left)
                float angle = atan2(pos.z(), pos.x());
                float radius = sqrt(pos.x()*pos.x() + pos.z()*pos.z());
                angle += 0.1f;
                m_camera->setPosition(QVector3D(cos(angle) * radius, pos.y(), sin(angle) * radius));
                m_camera->setViewCenter(QVector3D(0, 1, 0));
                return true;
            }
            if(keyEvent->key() == Qt::Key_Right){
                QVector3D pos = m_camera->position();
                // Rotate around Y axis (right)
                float angle = atan2(pos.z(), pos.x());
                float radius = sqrt(pos.x()*pos.x() + pos.z()*pos.z());
                angle -= 0.1f;
                m_camera->setPosition(QVector3D(cos(angle) * radius, pos.y(), sin(angle) * radius));
                m_camera->setViewCenter(QVector3D(0, 1, 0));
                return true;
            }
            if(keyEvent->key() == Qt::Key_Up){
                QVector3D pos = m_camera->position();
                // Move up
                m_camera->setPosition(QVector3D(pos.x(), pos.y() + 0.5f, pos.z()));
                m_camera->setViewCenter(QVector3D(0, 1, 0));
                return true;
            }
            if(keyEvent->key() == Qt::Key_Down){
                QVector3D pos = m_camera->position();
                // Move down
                m_camera->setPosition(QVector3D(pos.x(), pos.y() - 0.5f, pos.z()));
                m_camera->setViewCenter(QVector3D(0, 1, 0));
                return true;
            }
	    
            if(keyEvent->key() == Qt::Key_Space){
	      num_of_dice = 0;
	      m_counterText->setText(QString("Number of dice: %1").arg(num_of_dice));
	      m_resetSound->setAudioOutput(m_resetOutput);
	      m_resetSound->setSource(QUrl::fromLocalFile("reset_rack.mp3"));    
	      m_resetSound->play();
                for(auto &die : m_dice){
                    if(die.entity){
                        die.entity->deleteLater();
                    }
                }
                m_dice.clear();
                return true;
            }

	    if(keyEvent->key() == Qt::Key_Return){
	      QVector3D pos = m_camera->position();
	      m_camera->setPosition(QVector3D(0, 12, 8));
	      m_camera->setViewCenter(QVector3D(0, 1.5f, 0));
	      m_camera->setUpVector(QVector3D(0, 1, 0));
	      return true;
	    }
	    
            if(keyEvent->key() == Qt::Key_Escape){
                QCoreApplication::quit();
                return true;
            }
        }
        return false;
    }
private:
    QVector<FallingDice> &m_dice;
    Qt3DRender::QCamera *m_camera;
    Qt3DCore::QEntity *m_root;
    QMediaPlayer *m_resetSound;   
    QAudioOutput *m_resetOutput;
    Qt3DExtras::QExtrudedTextMesh *m_counterText;
};

app.installEventFilter(new KeyPressFilter(fallingDices,camera,rootEntity,reset_sound,reset_output, counterTextMesh));
 
    view.setRootEntity(rootEntity);
    view.show();
    
    return app.exec();
}
