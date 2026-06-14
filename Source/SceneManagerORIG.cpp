///////////////////////////////////////////////////////////////////////////////
// shadermanager.cpp
// ============
// manage the loading and rendering of 3D scenes
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "SceneManager.h"

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include <glm/gtx/transform.hpp>

// declaration of global variables
namespace
{
	const char* g_ModelName = "model";
	const char* g_ColorValueName = "objectColor";
	const char* g_TextureValueName = "objectTexture";
	const char* g_UseTextureName = "bUseTexture";
	const char* g_UseLightingName = "bUseLighting";
}

/***********************************************************
 *  SceneManager()
 *
 *  The constructor for the class
 ***********************************************************/
SceneManager::SceneManager(ShaderManager* pShaderManager)
{
	m_pShaderManager = pShaderManager;
	m_basicMeshes = new ShapeMeshes();

	// initialize the texture collection
	for (int i = 0; i < 16; i++)
	{
		m_textureIDs[i].tag = "/0";
		m_textureIDs[i].ID = -1;
	}
	m_loadedTextures = 0;
}

/***********************************************************
 *  ~SceneManager()
 *
 *  The destructor for the class
 ***********************************************************/
SceneManager::~SceneManager()
{
	m_pShaderManager = NULL;
	delete m_basicMeshes;
	m_basicMeshes = NULL;

	// destroy the created OpenGL textures
	DestroyGLTextures();
}

/***********************************************************
 *  CreateGLTexture()
 *
 *  This method is used for loading textures from image files,
 *  configuring the texture mapping parameters in OpenGL,
 *  generating the mipmaps, and loading the read texture into
 *  the next available texture slot in memory.
 ***********************************************************/
bool SceneManager::CreateGLTexture(const char* filename, std::string tag)
{
	int width = 0;
	int height = 0;
	int colorChannels = 0;
	GLuint textureID = 0;

	// indicate to always flip images vertically when loaded
	stbi_set_flip_vertically_on_load(true);

	// try to parse the image data from the specified image file
	unsigned char* image = stbi_load(
		filename,
		&width,
		&height,
		&colorChannels,
		0);

	// if the image was successfully read from the image file
	if (image)
	{
		std::cout << "Successfully loaded image:" << filename << ", width:" << width << ", height:" << height << ", channels:" << colorChannels << std::endl;

		glGenTextures(1, &textureID);
		glBindTexture(GL_TEXTURE_2D, textureID);

		// set the texture wrapping parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		// set texture filtering parameters
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// if the loaded image is in RGB format
		if (colorChannels == 3)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		// if the loaded image is in RGBA format - it supports transparency
		else if (colorChannels == 4)
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
		else
		{
			std::cout << "Not implemented to handle image with " << colorChannels << " channels" << std::endl;
			return false;
		}

		// generate the texture mipmaps for mapping textures to lower resolutions
		glGenerateMipmap(GL_TEXTURE_2D);

		// free the image data from local memory
		stbi_image_free(image);
		glBindTexture(GL_TEXTURE_2D, 0); // Unbind the texture

		// register the loaded texture and associate it with the special tag string
		m_textureIDs[m_loadedTextures].ID = textureID;
		m_textureIDs[m_loadedTextures].tag = tag;
		m_loadedTextures++;

		return true;
	}

	std::cout << "Could not load image:" << filename << std::endl;

	// Error loading the image
	return false;
}

/***********************************************************
 *  BindGLTextures()
 *
 *  This method is used for binding the loaded textures to
 *  OpenGL texture memory slots.  There are up to 16 slots.
 ***********************************************************/
void SceneManager::BindGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		// bind textures on corresponding texture units
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  DestroyGLTextures()
 *
 *  This method is used for freeing the memory in all the
 *  used texture memory slots.
 ***********************************************************/
void SceneManager::DestroyGLTextures()
{
	for (int i = 0; i < m_loadedTextures; i++)
	{
		glGenTextures(1, &m_textureIDs[i].ID);
	}
}

/***********************************************************
 *  FindTextureID()
 *
 *  This method is used for getting an ID for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureID(std::string tag)
{
	int textureID = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureID = m_textureIDs[index].ID;
			bFound = true;
		}
		else
			index++;
	}

	return(textureID);
}

/***********************************************************
 *  FindTextureSlot()
 *
 *  This method is used for getting a slot index for the previously
 *  loaded texture bitmap associated with the passed in tag.
 ***********************************************************/
int SceneManager::FindTextureSlot(std::string tag)
{
	int textureSlot = -1;
	int index = 0;
	bool bFound = false;

	while ((index < m_loadedTextures) && (bFound == false))
	{
		if (m_textureIDs[index].tag.compare(tag) == 0)
		{
			textureSlot = index;
			bFound = true;
		}
		else
			index++;
	}

	return(textureSlot);
}

/***********************************************************
 *  FindMaterial()
 *
 *  This method is used for getting a material from the previously
 *  defined materials list that is associated with the passed in tag.
 ***********************************************************/
bool SceneManager::FindMaterial(std::string tag, OBJECT_MATERIAL& material)
{
	if (m_objectMaterials.size() == 0)
	{
		return(false);
	}

	int index = 0;
	bool bFound = false;
	while ((index < m_objectMaterials.size()) && (bFound == false))
	{
		if (m_objectMaterials[index].tag.compare(tag) == 0)
		{
			bFound = true;
			material.ambientColor = m_objectMaterials[index].ambientColor;
			material.ambientStrength = m_objectMaterials[index].ambientStrength;
			material.diffuseColor = m_objectMaterials[index].diffuseColor;
			material.specularColor = m_objectMaterials[index].specularColor;
			material.shininess = m_objectMaterials[index].shininess;
		}
		else
		{
			index++;
		}
	}

	return(true);
}

/***********************************************************
 *  SetTransformations()
 *
 *  This method is used for setting the transform buffer
 *  using the passed in transformation values.
 ***********************************************************/
void SceneManager::SetTransformations(
	glm::vec3 scaleXYZ,
	float XrotationDegrees,
	float YrotationDegrees,
	float ZrotationDegrees,
	glm::vec3 positionXYZ)
{
	// variables for this method
	glm::mat4 modelView;
	glm::mat4 scale;
	glm::mat4 rotationX;
	glm::mat4 rotationY;
	glm::mat4 rotationZ;
	glm::mat4 translation;

	// set the scale value in the transform buffer
	scale = glm::scale(scaleXYZ);
	// set the rotation values in the transform buffer
	rotationX = glm::rotate(glm::radians(XrotationDegrees), glm::vec3(1.0f, 0.0f, 0.0f));
	rotationY = glm::rotate(glm::radians(YrotationDegrees), glm::vec3(0.0f, 1.0f, 0.0f));
	rotationZ = glm::rotate(glm::radians(ZrotationDegrees), glm::vec3(0.0f, 0.0f, 1.0f));
	// set the translation value in the transform buffer
	translation = glm::translate(positionXYZ);

	modelView = translation * rotationX * rotationY * rotationZ * scale;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setMat4Value(g_ModelName, modelView);
	}
}

/***********************************************************
 *  SetShaderColor()
 *
 *  This method is used for setting the passed in color
 *  into the shader for the next draw command
 ***********************************************************/
void SceneManager::SetShaderColor(
	float redColorValue,
	float greenColorValue,
	float blueColorValue,
	float alphaValue)
{
	// variables for this method
	glm::vec4 currentColor;

	currentColor.r = redColorValue;
	currentColor.g = greenColorValue;
	currentColor.b = blueColorValue;
	currentColor.a = alphaValue;

	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, false);
		m_pShaderManager->setVec4Value(g_ColorValueName, currentColor);
	}
}

/***********************************************************
 *  SetShaderTexture()
 *
 *  This method is used for setting the texture data
 *  associated with the passed in ID into the shader.
 ***********************************************************/
void SceneManager::SetShaderTexture(
	std::string textureTag)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setIntValue(g_UseTextureName, true);

		int textureID = -1;
		textureID = FindTextureSlot(textureTag);
		m_pShaderManager->setSampler2DValue(g_TextureValueName, textureID);
	}
}

/***********************************************************
 *  SetTextureUVScale()
 *
 *  This method is used for setting the texture UV scale
 *  values into the shader.
 ***********************************************************/
void SceneManager::SetTextureUVScale(float u, float v)
{
	if (NULL != m_pShaderManager)
	{
		m_pShaderManager->setVec2Value("UVscale", glm::vec2(u, v));
	}
}

/***********************************************************
 *  SetShaderMaterial()
 *
 *  This method is used for passing the material values
 *  into the shader.
 ***********************************************************/
void SceneManager::SetShaderMaterial(
	std::string materialTag)
{
	if (m_objectMaterials.size() > 0)
	{
		OBJECT_MATERIAL material;
		bool bReturn = false;

		bReturn = FindMaterial(materialTag, material);
		if (bReturn == true)
		{
			m_pShaderManager->setVec3Value("material.ambientColor", material.ambientColor);
			m_pShaderManager->setFloatValue("material.ambientStrength", material.ambientStrength);
			m_pShaderManager->setVec3Value("material.diffuseColor", material.diffuseColor);
			m_pShaderManager->setVec3Value("material.specularColor", material.specularColor);
			m_pShaderManager->setFloatValue("material.shininess", material.shininess);
		}
	}
}

/**************************************************************/
/*** STUDENTS CAN MODIFY the code in the methods BELOW for  ***/
/*** preparing and rendering their own 3D replicated scenes.***/
/*** Please refer to the code in the OpenGL sample project  ***/
/*** for assistance.                                        ***/
/**************************************************************/

/***********************************************************
 *  DefineObjectMaterials()
 *
 *  This method is used for configuring material settings
 *  for objects in the 3D scene.
 ***********************************************************/
void SceneManager::DefineObjectMaterials()
{
	// Define material for textured objects in the workstation scene
	OBJECT_MATERIAL workstationMaterial;
	workstationMaterial.ambientStrength = 0.20f;
	workstationMaterial.ambientColor = glm::vec3(0.45f, 0.45f, 0.45f);
	workstationMaterial.diffuseColor = glm::vec3(0.80f, 0.80f, 0.80f);

	// Low reflection keeps the desk from looking like a spotlight lens
	workstationMaterial.specularColor = glm::vec3(0.02f, 0.02f, 0.02f);
	workstationMaterial.shininess = 2.0f;

	workstationMaterial.tag = "workstation";

	// Add the material to the scene material collection
	m_objectMaterials.push_back(workstationMaterial);
}

/***********************************************************
 *  SetupSceneLights()
 *
 *  This method is called to add and configure the light
 *  sources for the 3D scene.
 ***********************************************************/
void SceneManager::SetupSceneLights()
{
	// Turn on custom lighting in the shader
	m_pShaderManager->setBoolValue(g_UseLightingName, true);

	// Main white light, widened to reduce the circular spotlight effect
	m_pShaderManager->setVec3Value("lightSources[0].position", -4.0f, 10.0f, 8.0f);
	m_pShaderManager->setVec3Value("lightSources[0].ambientColor", 0.25f, 0.25f, 0.25f);
	m_pShaderManager->setVec3Value("lightSources[0].diffuseColor", 0.25f, 0.25f, 0.25f);
	m_pShaderManager->setVec3Value("lightSources[0].specularColor", 0.10f, 0.10f, 0.10f);
	m_pShaderManager->setFloatValue("lightSources[0].focalStrength", 30.0f);
	m_pShaderManager->setFloatValue("lightSources[0].specularIntensity", 0.05f);

	// Secondary blue accent light
	m_pShaderManager->setVec3Value("lightSources[1].position", 4.0f, 3.0f, 6.0f);
	m_pShaderManager->setVec3Value("lightSources[1].ambientColor", 0.03f, 0.08f, 0.20f);
	m_pShaderManager->setVec3Value("lightSources[1].diffuseColor", 0.10f, 0.35f, 1.0f);
	m_pShaderManager->setVec3Value("lightSources[1].specularColor", 0.10f, 0.30f, 0.80f);
	m_pShaderManager->setFloatValue("lightSources[1].focalStrength", 8.0f);
	m_pShaderManager->setFloatValue("lightSources[1].specularIntensity", 0.25f);
}

/***********************************************************
 *  LoadSceneTextures()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the texture images into memory.
 ***********************************************************/
void SceneManager::LoadSceneTextures()
{
	// Load wood texture for the desk surface
	CreateGLTexture("Textures/wood.jpg", "wood");

	// Load black texture for the monitor frame and keyboard base
	CreateGLTexture("Textures/black.jpg", "black");

	// Load gray texture for the monitor body, stand, base, and keys
	CreateGLTexture("Textures/gray.jpg", "gray");

	// Load marble texture for the monitor screen
	CreateGLTexture("Textures/marble.jpg", "marble");

	// Load background texture for the wall
	CreateGLTexture("Textures/background.jpg", "background");

	// Bind all loaded textures to texture slots
	BindGLTextures();
}


/***********************************************************
 *  PrepareScene()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene
 *  rendering
 ***********************************************************/
void SceneManager::PrepareScene()
{
	// define materials for lighting on textured objects
	DefineObjectMaterials();

	// add and configure light sources for the scene
	SetupSceneLights();

	// load textures for the workstation scene
	LoadSceneTextures();

	// only one instance of a particular mesh needs to be
	// loaded in memory no matter how many times it is drawn
	// in the rendered 3D scene

	m_basicMeshes->LoadPlaneMesh();
	//Additional meshes can be loaded here as needed for the scene
	m_basicMeshes->LoadBoxMesh();
	m_basicMeshes->LoadCylinderMesh();
}

/***********************************************************
 *  RenderScene()
 *
 *  This method is used for rendering the 3D scene by
 *  transforming and drawing the basic 3D shapes
 ***********************************************************/
void SceneManager::RenderScene()
{
	// declare the variables for the transformations
	glm::vec3 scaleXYZ;
	float XrotationDegrees = 0.0f;
	float YrotationDegrees = 0.0f;
	float ZrotationDegrees = 0.0f;
	glm::vec3 positionXYZ;

	/*** Set needed transformations before drawing the basic mesh.  ***/
	/*** This same ordering of code should be used for transforming ***/
	/*** and drawing all the basic 3D shapes.						***/
	/******************************************************************/
	// set the XYZ scale for the mesh
	scaleXYZ = glm::vec3(10.0f, 1.0f, 6.0f); // changed table size to make the desk plane smaller

	// set the XYZ rotation for the mesh
	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// set the XYZ position for the mesh
	positionXYZ = glm::vec3(0.0f, 0.0f, 0.0f);

	// set the transformations into memory to be used on the drawn meshes
	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Apply tiled wood texture to the desk surface
	SetShaderTexture("wood");
	SetTextureUVScale(4.0f, 4.0f); // changed UV scale to fit the smaller desk

	// Apply material properties so lighting affects the smaller desk
	SetShaderMaterial("workstation"); // added lighting material to test the smaller desk plane

	// draw the mesh with transformation values
	m_basicMeshes->DrawPlaneMesh();
	/******************************************************************/

	// Back wall, light gray background behind the monitor
	//scaleXYZ = glm::vec3(20.0f, 1.0f, 10.0f);

	//XrotationDegrees = 90.0f;
	//YrotationDegrees = 0.0f;
	//ZrotationDegrees = 0.0f;

	//positionXYZ = glm::vec3(0.0f, 5.0f, -6.0f);

	//SetTransformations(
		//scaleXYZ,
		//XrotationDegrees,
		//YrotationDegrees,
		//ZrotationDegrees,
		//positionXYZ);

	// Apply gray color to the back wall
	//SetShaderColor(0.45f, 0.45f, 0.45f, 1.0f);

	// Disable lighting on the back wall to keep it flat and clean
	//m_pShaderManager->setBoolValue(g_UseLightingName, false);

	//m_basicMeshes->DrawPlaneMesh();

	// Turn lighting back on for the workstation objects
	//m_pShaderManager->setBoolValue(g_UseLightingName, true);

		/******************************************************************/
		// Monitor body, light gray rectangle behind the screen
		scaleXYZ = glm::vec3(7.0f, 4.6f, 0.25f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(0.0f, 4.0f, -2.0f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Apply gray texture to the monitor body
	SetShaderTexture("gray");
	SetTextureUVScale(1.0f, 1.0f);

	m_basicMeshes->DrawBoxMesh();
	/******************************************************************/

	/******************************************************************/
	// Screen display, white surface inside the monitor frame
	scaleXYZ = glm::vec3(5.2f, 1.0f, 2.9f);

	XrotationDegrees = 90.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(0.0f, 4.2f, -1.55f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Apply marble texture to the monitor screen
	SetShaderTexture("marble");
	SetTextureUVScale(1.0f, 1.0f);

	// Disable lighting on the screen so the texture stays visible
	m_pShaderManager->setBoolValue(g_UseLightingName, false);

	m_basicMeshes->DrawPlaneMesh();

	// Turn lighting back on for the rest of the monitor and keyboard
	m_pShaderManager->setBoolValue(g_UseLightingName, true);

	/******************************************************************/

	/******************************************************************/
	// Top monitor frame
	scaleXYZ = glm::vec3(10.6f, 0.18f, 0.18f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(0.0f, 7.14f, -1.45f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Apply black texture to the top monitor frame
	SetShaderTexture("black");
	SetTextureUVScale(1.0f, 1.0f);

	// Apply material properties so lighting affects the textured object
	SetShaderMaterial("workstation");

	m_basicMeshes->DrawBoxMesh();
	/******************************************************************/

	/******************************************************************/
	// Bottom monitor frame
	scaleXYZ = glm::vec3(10.6f, 0.18f, 0.18f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(0.0f, 1.44f, -1.45f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Apply black texture to the bottom monitor frame
	SetShaderTexture("black");
	SetTextureUVScale(1.0f, 1.0f);

	// Apply material properties so lighting affects the textured object
	SetShaderMaterial("workstation");

	m_basicMeshes->DrawBoxMesh();
	/******************************************************************/

	/******************************************************************/
	// Left monitor frame
	scaleXYZ = glm::vec3(0.18f, 5.7f, 0.18f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(-5.2f, 4.2f, -1.45f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Apply black texture to the left monitor frame
	SetShaderTexture("black");
	SetTextureUVScale(1.0f, 1.0f);

	// Apply material properties so lighting affects the textured object
	SetShaderMaterial("workstation");

	m_basicMeshes->DrawBoxMesh();
	/******************************************************************/

	/******************************************************************/
	// Right monitor frame
	scaleXYZ = glm::vec3(0.18f, 5.7f, 0.18f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(5.2f, 4.2f, -1.45f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Apply black texture to the right monitor frame
	SetShaderTexture("black");
	SetTextureUVScale(1.0f, 1.0f);

	// Apply material properties so lighting affects the textured object
	SetShaderMaterial("workstation");

	m_basicMeshes->DrawBoxMesh();
	/******************************************************************/

	/******************************************************************/
	// Monitor stand, vertical support below the screen
	scaleXYZ = glm::vec3(0.28f, -3.7f, 0.28f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(0.0f, 1.50f, -2.0f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Apply gray texture to the monitor stand
	SetShaderTexture("gray");
	SetTextureUVScale(1.0f, 1.0f);

	// Apply material properties so lighting affects the textured object
	SetShaderMaterial("workstation");

	m_basicMeshes->DrawCylinderMesh(true, true, true);
	/******************************************************************/

	/******************************************************************/
	// Monitor base, flat support at the bottom of the stand
	scaleXYZ = glm::vec3(2.0f, 0.22f, 1.2f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(0.0f, 0.15f, -2.0f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Apply black texture to the monitor base
	SetShaderTexture("black");
	SetTextureUVScale(1.0f, 1.0f);

	// Apply material properties so lighting affects the textured object
	SetShaderMaterial("workstation");

	m_basicMeshes->DrawBoxMesh();
	/******************************************************************/

	// Keyboard base, thin rectangular box placed in front of the monitor
	scaleXYZ = glm::vec3(4.8f, 0.15f, 1.0f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	positionXYZ = glm::vec3(0.0f, 0.12f, 2.0f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Apply black texture to the keyboard base
	SetShaderTexture("black");
	SetTextureUVScale(1.0f, 1.0f);

	// Apply material properties so lighting affects the textured object
	SetShaderMaterial("workstation");

	m_basicMeshes->DrawBoxMesh();
	/******************************************************************/

	/******************************************************************/
	// First row of keyboard keys
	scaleXYZ = glm::vec3(0.35f, 0.08f, 0.25f);

	XrotationDegrees = 0.0f;
	YrotationDegrees = 0.0f;
	ZrotationDegrees = 0.0f;

	// Apply gray texture to the keyboard keys
	SetShaderTexture("gray");
	SetTextureUVScale(1.0f, 1.0f);

	// Apply material properties so lighting affects the textured object
	SetShaderMaterial("workstation");

	positionXYZ = glm::vec3(-1.6f, 0.25f, 1.75f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	m_basicMeshes->DrawBoxMesh();

	positionXYZ = glm::vec3(-1.1f, 0.25f, 1.75f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	m_basicMeshes->DrawBoxMesh();

	positionXYZ = glm::vec3(-0.6f, 0.25f, 1.75f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	m_basicMeshes->DrawBoxMesh();

	positionXYZ = glm::vec3(-0.1f, 0.25f, 1.75f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	m_basicMeshes->DrawBoxMesh();

	positionXYZ = glm::vec3(0.4f, 0.25f, 1.75f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	m_basicMeshes->DrawBoxMesh();

	positionXYZ = glm::vec3(0.9f, 0.25f, 1.75f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	m_basicMeshes->DrawBoxMesh();

	positionXYZ = glm::vec3(1.4f, 0.25f, 1.75f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	m_basicMeshes->DrawBoxMesh();
	/******************************************************************/

	/******************************************************************/
	// Second row of keyboard keys
	positionXYZ = glm::vec3(-1.35f, 0.25f, 2.10f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	m_basicMeshes->DrawBoxMesh();

	positionXYZ = glm::vec3(-0.85f, 0.25f, 2.10f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	m_basicMeshes->DrawBoxMesh();

	positionXYZ = glm::vec3(-0.35f, 0.25f, 2.10f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	m_basicMeshes->DrawBoxMesh();

	positionXYZ = glm::vec3(0.15f, 0.25f, 2.10f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	m_basicMeshes->DrawBoxMesh();

	positionXYZ = glm::vec3(0.65f, 0.25f, 2.10f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	m_basicMeshes->DrawBoxMesh();

	positionXYZ = glm::vec3(1.15f, 0.25f, 2.10f);
	SetTransformations(scaleXYZ, XrotationDegrees, YrotationDegrees, ZrotationDegrees, positionXYZ);
	m_basicMeshes->DrawBoxMesh();
	/******************************************************************/

	/******************************************************************/
	// Spacebar, longer key on the front row
	scaleXYZ = glm::vec3(2.0f, 0.08f, 0.25f);

	positionXYZ = glm::vec3(0.0f, 0.25f, 2.45f);

	SetTransformations(
		scaleXYZ,
		XrotationDegrees,
		YrotationDegrees,
		ZrotationDegrees,
		positionXYZ);

	// Apply gray texture to the spacebar
	SetShaderTexture("gray");
	SetTextureUVScale(1.0f, 1.0f);

	// Apply material properties so lighting affects the textured object
	SetShaderMaterial("workstation");

	m_basicMeshes->DrawBoxMesh();
	/******************************************************************/
}