// sous mac 
// g++ -I/usr/local/include/ -I/usr/local/opt/opencv@2/include -I/usr/local/Cellar -lglfw -lGLEW -lopencv_core -lopencv_imgproc -lopencv_highgui -lopencv_imgcodecs main.cpp -framework OpenGL -omain
// ./main6

// sous linux 	
// g++ -I/usr/local/include/ -I/public/ig/glm/ -c main6.cpp  -omain6.o
// g++ -I/usr/local main6.o -lglfw  -lGLEW  -lGL -lopencv_core -lopencv_imgproc -lopencv_highgui  -lopencv_imgcodecs -omain6
// ./main6

// Inclut les en-têtes standards


//VISUALISATION
//H = 20/2
//L = 41 proportionnelle

//Echelle regle de 2
//Hauteur exploiquier dans le sujet
//Indice i*41+j




#include <stdio.h>
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <math.h>
using namespace std;

#include <stdlib.h>
#include <string.h>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#ifdef __APPLE__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
using namespace cv;

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace glm;


// Le nombre de donnees
const int taille = 18 * 16 * 8;
const int N = 41 * 20;
vector<int> pts(41*20);
vector<int> ranks(41*20);

GLfloat color[] = {
    //Couleur pour la premiere equipe
    0.0f, 0.0f, 1.0f,
    
    //Couleur pour les 3 prochaines
    0.4f, 1.0f, 1.0f,
    
    //Couleur pour les 3 prochaines
    1.0f, 1.0f, 0.6f,
    
    //Couleur pour les 3 prochaines
    0.75f, 0.75f, 0.75f,
    
    //Couleur pour les 6 prochaines
    0.2f, 0.2f, 0.2f,
    
    //Couleur pour le reste
    1.0f, 0.6f, 0.8f
};

// Avec 3 parties dans le VBO ca commence a etre complique. On va stocker la taille
// des 3 parties dans des variables pour plus facilement calculer les decallages
long vertexSize, colorSize, texCoordSize, normalSize;

// Les donnees sous forme de pointeur. Il faudra donc faire un malloc
// pour allouer de l'espace avant de mettre des donnees dedans
GLfloat *gVertexBufferData   = NULL;
GLfloat *gVertexColorData    = NULL;
GLfloat *gVertexTexcoordData = NULL;
GLfloat *gVertexNormalData = NULL;

// l'angle de rotation de la camera autour du modele
float angleRot = 0.0f;

// LA fenetre
GLFWwindow* window; 

// La texture d'alphabet
GLuint texId;

// la vitesse initiale
float speed = 0.0;

// La taille de notre fenetre
int winWidth  = 1200;
int winHeight = 800;

// La taille de notre texture
int texWidth  = -1;
int texHeight = -1;

// This will identify our vertex buffer (VBO)
GLuint vertexbuffer;

// Identifiant de notre VAO
GLuint vertexArrayID;

// identifiant de notre programme de shaders
GLuint programID;


// stocke les variables uniformes qui seront communes a tous les vertex dessines
GLint uniform_proj, uniform_view, uniform_model, uniform_texture, uniform_lightPos;


void loadData(string fn, vector<int> &pts, vector<int> &ranks){
    ifstream file (fn); // declare file stream: http://www.cplusplus.com/reference/iostream/ifstream/
    string value;
//    int count = 0;
    
    //Nombre d'équipes
    for(int j = 0; j < 20; j++){
        
        //Lis le nom de l'équipe
        getline ( file, value, ',' ); // read a string until next comma: http://www.cplusplus.com/reference/string/getline/
        string teamName = string( value, 0, value.length() );
        
        //Nombre de journées
        for(int i = 0; i < 41; i++){
            //Le rang
            getline ( file, value, ',' );
            string rank = string( value, 0, value.length() );
            int r = stoi(rank, NULL);
            ranks[j*41+i] = r;
            
            //Les points
            getline ( file, value, ',' );
            string points = string( value, 0, value.length() );
            int p = stoi(points, NULL);
            pts[j*41+i] = p;
            
            //Match --> nom de l'équipe à domicile
            getline ( file, value, ',' );
            string homeTeam = string( value, 0, value.length() );

            
            //Nombre de buts pour l'équipe à domicile
            getline ( file, value, ',' );
            string homeGoal = string( value, 0, value.length() );
            int hGoal = stoi(homeGoal, NULL);
            
            //Nombre de buts pour l'équipe à l'extérieur
            getline ( file, value, ',' );
            string awayGoal = string( value, 0, value.length() );
            int aGoal = stoi(awayGoal, NULL);
            
            //Match --> nom de l'équipe à l'extérieur
            getline ( file, value, ',' );
            string awayTeam = string( value, 0, value.length() );
        }
        getline ( file, value, '\n' );
    }
    
}



// Charge une texture et retourne l'identifiant openGL
GLuint LoadTexture(string fileName){
    GLuint tId = -1;
    // On utilise OpenCV pour charger l'image
    Mat image = imread(fileName, CV_LOAD_IMAGE_UNCHANGED);
    
    // On va utiliser des TEXTURE_RECTANGLE au lieu de classiques TEXTURE_2D
    // car avec ca les coordonnees de texture s'exprime en pixels  et non en coordoonnes homogenes (0.0...1.0)
    // En effet la texture est composee de lettres et symbole que nous voudrons extraire... or la position et
    // taille de ces symboles dans la texture sont connuees en "pixels". Ca sera donc plus facile
    
    //comme d'hab on fait generer un numero unique(ID) par OpenGL
    glGenTextures(1, &tId);
    
    texWidth  = image.cols;
    texHeight = image.rows;
    
    glBindTexture(GL_TEXTURE_RECTANGLE, tId);
    // on envoie les pixels a la carte graphique
    glTexImage2D(GL_TEXTURE_RECTANGLE,
                 0,           // mipmap level => Attention pas de mipmap dans les textures rectangle
                 GL_RGBA,     // internal color format
                 image.cols,
                 image.rows,
                 0,           // border width in pixels
                 GL_BGRA,     // input file format. Arg le png code les canaux dans l'autre sens
                 GL_UNSIGNED_BYTE, // image data type
                 image.ptr());
    // On peut donner des indication a opengl sur comment la texture sera utilisee
    glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    
    // INTERDIT sur les textures rectangle!
    //glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_RECTANGLE, 0);
    
    return tId;
}


// Charge un programme de shaders, le compile et recupere dedans des pointeurs vers
// les variables homogenes que nous voudront mettre a jour plus tard, a chaque dessin 
GLuint LoadShaders(const char * vertex_file_path,const char * fragment_file_path){
    
    // Create the shaders
    GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
    GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);
    
    // Read the Vertex Shader code from the file
    string VertexShaderCode;
    ifstream VertexShaderStream(vertex_file_path, ios::in);
    if(VertexShaderStream.is_open()){
        string Line = "";
        while(getline(VertexShaderStream, Line))
            VertexShaderCode += "\n" + Line;
        VertexShaderStream.close();
    }else{
        printf("Impossible to open %s. Are you in the right directory ? Don't forget to read the FAQ !\n", vertex_file_path);
        getchar();
        return 0;
    }
    
    // Read the Fragment Shader code from the file
    string FragmentShaderCode;
    ifstream FragmentShaderStream(fragment_file_path, ios::in);
    if(FragmentShaderStream.is_open()){
        string Line = "";
        while(getline(FragmentShaderStream, Line))
            FragmentShaderCode += "\n" + Line;
        FragmentShaderStream.close();
    }
    
    GLint Result = GL_FALSE;
    int InfoLogLength;
    
    
    // Compile Vertex Shader
    printf("Compiling shader : %s\n", vertex_file_path);
    char const * VertexSourcePointer = VertexShaderCode.c_str();
    glShaderSource(VertexShaderID, 1, &VertexSourcePointer , NULL);
    glCompileShader(VertexShaderID);
    
    // Check Vertex Shader
    glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
    glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
    if ( InfoLogLength > 0 ){
        vector<char> VertexShaderErrorMessage(InfoLogLength+1);
        glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
        printf("%s\n", &VertexShaderErrorMessage[0]);
    }
    
    
    
    // Compile Fragment Shader
    printf("Compiling shader : %s\n", fragment_file_path);
    char const * FragmentSourcePointer = FragmentShaderCode.c_str();
    glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer , NULL);
    glCompileShader(FragmentShaderID);
    
    // Check Fragment Shader
    glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
    glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
    if ( InfoLogLength > 0 ){
        vector<char> FragmentShaderErrorMessage(InfoLogLength+1);
        glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
        printf("%s\n", &FragmentShaderErrorMessage[0]);
    }
    
    
    // Link the program
    printf("Linking program\n");
    GLuint progID = glCreateProgram();
    glAttachShader(progID, VertexShaderID);
    glAttachShader(progID, FragmentShaderID);
    glLinkProgram(progID);
    
    
    // Check the program
    glGetProgramiv(progID, GL_LINK_STATUS, &Result);
    glGetProgramiv(progID, GL_INFO_LOG_LENGTH, &InfoLogLength);
    if ( InfoLogLength > 0 ){
        vector<char> ProgramErrorMessage(InfoLogLength+1);
        glGetProgramInfoLog(progID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
        printf("%s\n", &ProgramErrorMessage[0]);
    }
    
    
    glDetachShader(progID, VertexShaderID);
    glDetachShader(progID, FragmentShaderID);
    
    glDeleteShader(VertexShaderID);
    glDeleteShader(FragmentShaderID);
    
    return progID;
}


//Angle de rotation dépendant de l'axe
float rotAngleY = 0.0f;
float rotAngleX = 0.0f;


void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods){
    if (key== GLFW_KEY_LEFT && action == GLFW_PRESS ){
        //Augmente l'angle de roation si on appuie sur "LEFT"
        rotAngleY = (rotAngleY+M_PI/100);
    } else if (key== GLFW_KEY_RIGHT && action == GLFW_PRESS){
        //Augmente l'angle de roation si on appuie sur "RIGHT"
        rotAngleY = (rotAngleY-M_PI/100);
    } else if (key== GLFW_KEY_UP && action == GLFW_PRESS){
        //Augmente l'angle de roation si on appuie sur "UP"
        rotAngleX = (rotAngleX+M_PI/100);
    } else if (key == GLFW_KEY_DOWN && action == GLFW_PRESS){
        //Augmente l'angle de roation si on appuie sur "DOWN"
        rotAngleX = (rotAngleX-M_PI/100);
    }
    
}




void generateData(vector<int> point, vector<int> rang){
    //On initialise 3 variables qui nous serviront pour les couleurs
    GLfloat colorR;
    GLfloat colorG;
    GLfloat colorB;
    
    //Size of our data
    vertexSize = N * taille* sizeof(GLfloat);
    colorSize = N * taille* sizeof(GLfloat);
    normalSize = N * taille* sizeof(GLfloat);
    
    if (gVertexBufferData != NULL)
        free(gVertexBufferData);
    
    if (gVertexColorData!= NULL)
        free(gVertexColorData);
    
    if(gVertexNormalData != NULL)
        free(gVertexNormalData);
    
    //Memory allocation
    gVertexBufferData = (GLfloat*)malloc(vertexSize);
    gVertexColorData = (GLfloat*)malloc(colorSize);
    gVertexNormalData = (GLfloat*)malloc(normalSize);
    //    ((((19 - rang[i * 41 + j]) / 19.0) + (point[i * 41 + j] / 98.0)) / 2.0) + ((1.0/2.0)/20.0);
    float r = ((1.0 / 2.0)/20.0)/2.0;
    
    //Nombre d'équipe
    for(int i = 0; i < 20; i++){
        //Nombre de jours
        for(int j = 0; j < 40; j++){
            float hauteur         = ((((19 - rang[i * 41 + j]) / 19.0)     + (point[i * 41 + j] / 98.0)) / 2.0);
            float hauteurProchain = ((((19 - rang[i * 41 + (j+1)]) / 19.0) + (point[i * 41 + (j+1)] / 98.0)) / 2.0);
            float r2 = rang[i * 41 + (j+1)] - rang[i * 41 + j];
            //Pour la 3d
            for(int k = 0; k < 16; k++){ //Pour faire les cylindre
                //"Coupe" les rectanlges
                float theta = k*M_PI/16;
                for(int l = 0; l < 8; l++){ //Pour faire une équipe passé devant ou derrière
                    //"Recoupe" les rectangles
                    
                    
                    //First Triangle
                    //Top left
                    gVertexBufferData[i * 41 *taille+ j * taille+ k*18*8+l*8*18] = j/41.; //X
                    gVertexBufferData[i * 41 *taille+ j * taille+ k*18*8+l*8*18+ 1] = hauteur + 1*r*cos(theta); //Y
                    gVertexBufferData[i * 41 *taille+ j * taille+ k*18*8+l*8*18+ 2] = r*sin(theta)+ r2*sin(l*M_PI/8); //Z
                    
                    //Bottom left
                    gVertexBufferData[i * 41 *taille+ j * taille+ k*18*8+l*8*18+ 3] = j/41.; //X
                    gVertexBufferData[i * 41 *taille+ j * taille+ k*18*8+l*8*18+ 4] = hauteur + 1*r*cos((theta+(M_PI/16))); //Y
                    gVertexBufferData[i * 41 *taille+ j * taille+ k*18*8+l*8*18+ 5] = r*sin((theta+(M_PI/16)))+r2*sin(l*M_PI/8); //Z
                
                    //Top right
                    gVertexBufferData[i * 41 *taille+ j * taille+ k*18*8+l*8*18+ 6] = (j+1)/41.; //X
                    gVertexBufferData[i * 41 *taille+ j * taille+ k*18*8+l*8*18+ 7] = hauteurProchain + 1*r*cos(theta); //Y
                    gVertexBufferData[i * 41 *taille+ j * taille+ k*18*8+l*8*18+ 8] = r*sin(theta)+r2*sin((l*M_PI/8)); //Z
                    
                    //Second Triangle
                    //Top right
                    gVertexBufferData[i * 41 *taille+ j * taille+ k*18*8+l*8*18+ 9] = (j+1)/41.; //X
                    gVertexBufferData[i * 41 *taille+ j * taille+ k*18*8+l*8*18+ 10] = hauteurProchain + 1*r*cos(theta); //Y
                    gVertexBufferData[i * 41 *taille+ j * taille+ k*18*8+l*8*18+ 11] = r*sin(theta)+r2*sin((l*M_PI/8)); //Z

                    //Bottom left
                    gVertexBufferData[i * 41 *taille+ j * taille+ k*18*8+l*8*18+ 12] = j/41.; //X
                    gVertexBufferData[i * 41 *taille+ j * taille+ k*18*8+l*8*18+ 13] = hauteur + 1*r*cos((theta+(M_PI/16))); //Y
                    gVertexBufferData[i * 41 *taille+ j * taille+ k*18*8+l*8*18+ 14] = r*sin((theta+(M_PI/16)))+r2*sin(l*M_PI/8); //Z

                    //Bottom right
                    gVertexBufferData[i * 41 *taille+ j * taille+ k*18*8+l*8*18+ 15] = (j+1)/41.; //X
                    gVertexBufferData[i * 41 *taille+ j * taille+ k*18*8+l*8*18+ 16] = hauteurProchain + 1*r*cos((theta+(M_PI/16))); //Y
                    gVertexBufferData[i * 41 *taille+ j * taille+ k*18*8+l*8*18+ 17] = r*sin((theta+(M_PI/16)))+r2*sin((l*M_PI/8)); //Z
                    
//                    //Bottom right
//                    gVertexBufferData[i * 41 *taille+ j * taille+ k*12*8+l*8*12+ 9] = (j+1)/41.; //X
//                    gVertexBufferData[i * 41 *taille+ j * taille+ k*12*8+l*8*12+ 10] = hauteurProchain + 1*r*cos((theta+(M_PI/16))); //Y
//                    gVertexBufferData[i * 41 *taille+ j * taille+ k*12*8+l*8*12+ 11] = r*sin((theta+(M_PI/16)))+r2*sin((l*M_PI/8)); //Z
                    
                    
                    //Selecting the color according to the team's rank
                    if(rang[i*41+40] == 0){
                        colorR = color[0];
                        colorG = color[1];
                        colorB = color[2];
                    } else if(rang[i*41+40] > 0 && rang[i*41+40] < 4){
                        colorR = color[3];
                        colorG = color[4];
                        colorB = color[5];
                    } else if(rang[i*41+40] >= 4 && rang[i*41+40] < 7){
                        colorR = color[6];
                        colorG = color[7];
                        colorB = color[8];
                    } else if(rang[i*41+40] >= 7 && rang[i*41+40] < 10){
                        colorR = color[9];
                        colorG = color[10];
                        colorB = color[11];
                    } else if(rang[i*41+40] >= 10 && rang[i*41+40] < 16){
                        colorR = color[12];
                        colorG = color[13];
                        colorB = color[14];
                    } else {
                        colorR = color[15];
                        colorG = color[16];
                        colorB = color[17];
                    }
                    
                    //Colors
                    gVertexColorData[i * 41 *taille+ j * taille + k*18*8+ l*8*18] = colorR; //R
                    gVertexColorData[i * 41 *taille+ j * taille + k*18*8+ l*8*18+1] = colorG; //G
                    gVertexColorData[i * 41 *taille+ j * taille + k*18*8+ l*8*18+2] = colorB; //B
            
                    gVertexColorData[i * 41 *taille+ j * taille + k*18*8+ l*8*18+3] = colorR; //R
                    gVertexColorData[i * 41 *taille+ j * taille + k*18*8+ l*8*18+4] = colorG; //G
                    gVertexColorData[i * 41 *taille+ j * taille + k*18*8+ l*8*18+5] = colorB; //B
                
                    gVertexColorData[i * 41 *taille+ j * taille + k*18*8+ l*8*18+6] = colorR; //R
                    gVertexColorData[i * 41 *taille+ j * taille + k*18*8+ l*8*18+7] = colorG; //G
                    gVertexColorData[i * 41 *taille+ j * taille + k*18*8+ l*8*18+8] = colorB; //B
                    
                    gVertexColorData[i * 41 *taille+ j * taille + k*18*8+ l*8*18+9] = colorR; //R
                    gVertexColorData[i * 41 *taille+ j * taille + k*18*8+ l*8*18+10] = colorG; //G
                    gVertexColorData[i * 41 *taille+ j * taille + k*18*8+ l*8*18+11] = colorB; //B

                    gVertexColorData[i * 41 *taille+ j * taille + k*18*8+ l*8*18+12] = colorR; //R
                    gVertexColorData[i * 41 *taille+ j * taille + k*18*8+ l*8*18+13] = colorG; //G
                    gVertexColorData[i * 41 *taille+ j * taille + k*18*8+ l*8*18+14] = colorB; //B

                    gVertexColorData[i * 41 *taille+ j * taille + k*18*8+ l*8*18+15] = colorR; //R
                    gVertexColorData[i * 41 *taille+ j * taille + k*18*8+ l*8*18+16] = colorG; //G
                    gVertexColorData[i * 41 *taille+ j * taille + k*18*8+ l*8*18+17] = colorB; //B
                    
                    
                    //Normals
                    gVertexNormalData[i * 41 *taille+ j * taille + k*18*8+l*8*18] = 0; //R
                    gVertexNormalData[i * 41 *taille+ j * taille + k*18*8+ l*8*18+1] = cos(theta); //G
                    gVertexNormalData[i * 41 *taille+ j * taille + k*18*8+ l*8*18+2] = sin(theta); //B
                
                    gVertexNormalData[i * 41 *taille+ j * taille + k*18*8+ l*8*18+3] = 0; //R
                    gVertexNormalData[i * 41 *taille+ j * taille + k*18*8+ l*8*18+4] = cos((theta+(M_PI/16))); //G
                    gVertexNormalData[i * 41 *taille+ j * taille + k*18*8+ l*8*18+5] = sin((theta+(M_PI/16))); //B
                
                    gVertexNormalData[i * 41 *taille+ j * taille + k*18*8+ l*8*18+6] = 0; //R
                    gVertexNormalData[i * 41 *taille+ j * taille + k*18*8+ l*8*18+7] = cos(theta); //G
                    gVertexNormalData[i * 41 *taille+ j * taille + k*18*8+ l*8*18+8] = sin(theta); //B
                    
                    gVertexNormalData[i * 41 *taille+ j * taille + k*18*8+ l*8*18+9] = 0; //R
                    gVertexNormalData[i * 41 *taille+ j * taille + k*18*8+ l*8*18+10] = cos(theta); //G
                    gVertexNormalData[i * 41 *taille+ j * taille + k*18*8+ l*8*18+11] = sin(theta); //B

                    gVertexNormalData[i * 41 *taille+ j * taille + k*18*8+ l*8*18+12] = 0; //R
                    gVertexNormalData[i * 41 *taille+ j * taille + k*18*8+ l*8*18+13] = cos((theta+(M_PI/16))); //G
                    gVertexNormalData[i * 41 *taille+ j * taille + k*18*8+ l*8*18+14] = sin((theta+(M_PI/16))); //B

                    gVertexNormalData[i * 41 *taille+ j * taille + k*18*8+ l*8*18+15] = 0; //R
                    gVertexNormalData[i * 41 *taille+ j * taille + k*18*8+ l*8*18+16] = cos((theta+(M_PI/16))); //G
                    gVertexNormalData[i * 41 *taille+ j * taille + k*18*8+ l*8*18+17] = sin((theta+(M_PI/16))); //B
                    
                    
                }
            }
        }
    }
    
}


void initOpenGL(){
    // Enable depth test
//    glEnable(GL_DEPTH_TEST);
    
    // Accept fragment if it closer to the camera than the former one
    glDepthFunc(GL_LESS);
    glDepthRange(-1, 1);
    
    // creation du glVertexAttribPointer
    glGenVertexArrays(1, &vertexArrayID);
    glBindVertexArray(vertexArrayID);
    
    
    // Generate 1 buffer, put the resulting identifier in vertexbuffer
    glGenBuffers(1, &vertexbuffer);
    
    // The following commands will talk about our 'vertexbuffer' buffer
    glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
    
    // Only allocate memory. Do not send yet our vertices to OpenGL.
    glBufferData(GL_ARRAY_BUFFER, vertexSize+colorSize+normalSize, 0, GL_STATIC_DRAW);
    
    // send vertices in the first part of the buffer
    glBufferSubData(GL_ARRAY_BUFFER, 0,                            vertexSize, gVertexBufferData);
    
    // send colors in the second part of the buffer
    glBufferSubData(GL_ARRAY_BUFFER, vertexSize, colorSize, gVertexColorData);
    
    //send normal in the thirf part of the buffer
    glBufferSubData(GL_ARRAY_BUFFER, vertexSize+colorSize, normalSize, gVertexNormalData);
    
    // send tex coords in the third part of the buffer
    //glBufferSubData(GL_ARRAY_BUFFER, vertexSize+colorSize, texCoordSize, gVertexTexcoordData);
    
    // ici les commandes stockees "une fois pour toute" dans le VAO
    // avant on faisait ca a chaque dessin
    glVertexAttribPointer(
                          0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
                          3,                  // size
                          GL_FLOAT,           // type
                          GL_FALSE,           // normalized?
                          0,                  // stride
                          (void*)0            // array buffer offset
                          );
    glEnableVertexAttribArray(0);
    
    glVertexAttribPointer( // same thing for the colors
                          1,
                          3,
                          GL_FLOAT,
                          GL_FALSE,
                          0,
                          (void*)vertexSize);
    glEnableVertexAttribArray(1);
    
    glVertexAttribPointer( // same thing for the normals
                          2,
                          3,
                          GL_FLOAT,
                          GL_FALSE,
                          0,
                          (void*)(vertexSize+colorSize));
    glEnableVertexAttribArray(2);
    
//    glVertexAttribPointer(
//                          2,
//                          2,
//                          GL_FLOAT,
//                          GL_FALSE,
//                          0,
//                          (void*)(vertexSize+colorSize));
//    glEnableVertexAttribArray(2);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    
    // on desactive le VAO a la fin de l'initialisation
    glBindVertexArray (0);
}


GLFWwindow *initMainwindow(){
    // Nous allons apprendre a lire une texture de "symboles" generee a partir d'un outil comme :
    // https://evanw.github.io/font-texture-generator/
    
    glfwWindowHint(GLFW_SAMPLES, 4); // 4x antialiasing
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3); // On veut OpenGL 3.3
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // Pour rendre MacOS heureux ; ne devrait pas être nécessaire
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE); // On ne veut pas l'ancien OpenGL
    glfwWindowHint(GLFW_DEPTH_BITS, 24);
    
    // Ouvre une fenêtre et crée son contexte OpenGl
    GLFWwindow *win = glfwCreateWindow( winWidth, winHeight, "Main 06", NULL, NULL);
    if( win == NULL ){
        fprintf( stderr, "Failed to open GLFW window. If you have an Intel GPU, they are maybe not 3.3 compatible. \n" );
        glfwTerminate();
    }
    
    //
    glfwMakeContextCurrent(win);
    
    // Assure que l'on peut capturer la touche d'échappement
    glfwSetInputMode(win, GLFW_STICKY_KEYS, GL_TRUE);
    
    // active une callback = une fonction appellee automatiquement quand un evenement arrive
    glfwSetKeyCallback(win, key_callback);
    
    return win;
}


void draw(){
    
    // clear before every draw
    glClearColor(0.9, 0.9, 0.9, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Use our shader
    glUseProgram(programID);
    
    //  matrice de projection proportionelle a la taille de la fenetre
    mat4 projectionMatrix = glm::ortho( -0.1f, 1.1f, -0.1f, 1.1f, -1.f, 1.f );
    mat4 viewMatrix       = lookAt(
                                   vec3(0.0, 0.0, 0.25), // where is the camara
                                   vec3(0.0, 0.0, 0.0), //where it looks
                                   vec3(0.0, 1.0, 0.0) // head is up
                                   );
    mat4 modelMatrix      = glm::mat4(1.0f);
    //On multiple la matrice pour garder les rotations sur les deux axes
    modelMatrix *= glm::rotate(modelMatrix, rotAngleY, vec3(0.0f, 1.0f, 0.0f));
    modelMatrix *= glm::rotate(modelMatrix, rotAngleX, vec3(1.0f, 0.0f, 0.0f));
    
    glUniformMatrix4fv(uniform_proj,  1, GL_FALSE, value_ptr(projectionMatrix));
    glUniformMatrix4fv(uniform_view,  1, GL_FALSE, value_ptr(viewMatrix));
    glUniformMatrix4fv(uniform_model, 1, GL_FALSE, value_ptr(modelMatrix));
    
    // La texture aussi est donnee en variable uniforme. On lui donne le No 0
    //glUniform1i(uniform_texture, 0);
    
        // on re-active le VAO avant d'envoyer les buffers
        glBindVertexArray(vertexArrayID);
        
        // On active la texture 0
        //    glActiveTexture(GL_TEXTURE0);
        
        // Verrouillage de la texture
        //glBindTexture(GL_TEXTURE_RECTANGLE, texId);
        
        // Draw the triangle(s) !
        glDrawArrays(GL_TRIANGLES, 0, (N*taille)/3);
        
        // Déverrouillage de la texture
        //    glBindTexture(GL_TEXTURE_RECTANGLE, 0);
        
        // on desactive le VAO a la fin du dessin
        glBindVertexArray (0);
    
    // on desactive les shaders
    glUseProgram(0);
    
    // Swap buffers
    glfwSwapBuffers(window);
    glfwPollEvents();
}


int main(){
    
    
    loadData("DataProject2019/data/rankspts.csv", pts, ranks);
    generateData(pts, ranks);
//    for(int i = 0; i < 20; i++){
//        for(int j = 0; j < 40; j++){
//            cout << ranks[i*41 + j] << endl;
//        }
//        cout << endl;
//    }
    
    // Initialise GLFW
    if( !glfwInit() ) {
        fprintf( stderr, "Failed to initialize GLFW\n" );
        return -1;
    }
    
    window = initMainwindow();
    
    // Initialise GLEW
    glewExperimental=true; // Nécessaire dans le profil de base
    if (glewInit() != GLEW_OK) {
        fprintf(stderr, "Failed to initialize GLEW\n");
        return -1;
    }
    
    
    initOpenGL();
    
    
    programID = LoadShaders( "SimpleVertexShader6.vertexshader", "SimpleFragmentShader6.fragmentshader" );
    uniform_proj    = glGetUniformLocation(programID, "projectionMatrix");
    uniform_view    = glGetUniformLocation(programID, "viewMatrix");
    uniform_model   = glGetUniformLocation(programID, "modelMatrix");
    uniform_texture = glGetUniformLocation(programID, "loctexture");

    do{
        draw();
    } // Vérifie si on a appuyé sur la touche échap (ESC) ou si la fenêtre a été fermée
    while( glfwGetKey(window, GLFW_KEY_ESCAPE ) != GLFW_PRESS && glfwWindowShouldClose(window) == 0 );
}
