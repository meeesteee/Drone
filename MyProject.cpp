// This has been adapted from the Vulkan tutorial

#include "MyProject.hpp"

//gubo
struct globalUniformBufferObject {
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
};
//ubo
struct UniformBufferObject {
	alignas(16) glm::mat4 model;
};


// folder paths
const std::string TEXTURE_PATH = "textures/";
const std::string MODEL_PATH = "models/";
const std::string SHADER_PATH = "shaders/";


unsigned char *texTerrain;
int texTerrainWidth, texTerrainHeight;	// number of pixels (1018x1024)

// height map max values
const float xTexTerrain = 1661;
const float zTexTerrain = 1661;
const float minHeight = 107;
const float maxHeight = -127;

static std::vector<float> vertices;

// per altezza pixel
const int MINY = 27;
const  int MAXY = 144;



bool findY(double x, double y, double z, int height, int width) {
	// x and z are the current position of the map
	// height and width are the ones of the heightmap
	int pixHeight;
	int pixWidth;
	
	pixWidth = (int)floor((x + xTexTerrain) * (width) / (2 * xTexTerrain));
	pixHeight = (int) floor((z + zTexTerrain) * (height) / (2 * zTexTerrain));

	int index = 3 * width * pixWidth + 3 * pixHeight;

	int k = MAXY - vertices[index+1] + 1;
	float yMap = maxHeight + k * (minHeight - maxHeight) / (MAXY - MINY);

	//if (stamp) {
	//	std::cout << pixWidth << "\n";
	//	std::cout << pixHeight << "\n";
	//	std::cout << index << "\n";
	//	std::cout << vertices[index] << "\t" << vertices[index + 1] << "\t" << vertices[index + 2] << "\n";
	//	std::cout << yMap << "\n";

	//}
	

	if (y + 10> yMap)		// offset 10
		return true;
	else return false;
	
}


// MAIN ! 
class MyProject : public BaseProject {
protected:
	// Here you list all the Vulkan objects you need:
	bool change = false;
	// Descriptor Layouts [what will be passed to the shaders]
	DescriptorSetLayout DSLglobal;
	DescriptorSetLayout DSLobj;

	// Pipelines [Shader couples]
	Pipeline P1;

	// Models, textures and Descriptors (values assigned to the uniforms)
	// ground
	Model M_ground;
	Texture T_ground;
	DescriptorSet DS_ground;	// instance DSLobj

	// drone
	Model M_drone;
	Texture T_drone;
	DescriptorSet DS_drone;	// instance DSLobj


	DescriptorSet DS_global;

	// Here you set the main application parameters
	void setWindowParameters() {
		// window size, titile and initial background
		windowWidth = 800; //1920
		windowHeight = 600; //1080
		windowTitle = "Drone";
		initialBackgroundColor = { 54 / 255.0, 168.0 / 255.0, 255.0 / 255.0 };

		// Descriptor pool sizes
		uniformBlocksInPool = 3;
		texturesInPool = 2;
		setsInPool = 3;//descriptor set
	}



	// Here you load and setup all your Vulkan objects
	void localInit() {
		// Descriptor Layouts [what will be passed to the shaders]
		DSLobj.init(this, {
			// this array contains the binding:
			// first  element : the binding number
			// second element : the time of element (buffer or texture)
			// third  element : the pipeline stage where it will be used
			{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT},
			{1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT}
			});

		DSLglobal.init(this, {
					{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS},
			});

		// Pipelines [Shader couples]
		// The last array, is a vector of pointer to the layouts of the sets that will
		// be used in this pipeline. The first element will be set 0, and so on..
		P1.init(this, (SHADER_PATH + "vert.spv").c_str(),
			(SHADER_PATH + "frag.spv").c_str(), { &DSLglobal, &DSLobj });


		// Models, textures and Descriptors (values assigned to the uniforms)
		M_ground.init(this, (MODEL_PATH + "Terrain.obj").c_str());
		T_ground.init(this, (TEXTURE_PATH + "PaloDuroPark1.jpg").c_str());
		DS_ground.init(this, &DSLobj, {
			// the second parameter, is a pointer to the Uniform Set Layout of this set
			// the last parameter is an array, with one element per binding of the set.
			// first  elmenet : the binding number
			// second element : UNIFORM or TEXTURE (an enum) depending on the type
			// third  element : only for UNIFORMs, the size of the corresponding C++ object
			// fourth element : only for TEXTUREs, the pointer to the corresponding texture object
						{0, UNIFORM, sizeof(UniformBufferObject), nullptr},
						{1, TEXTURE, 0, &T_ground}
			});

		M_drone.init(this, (MODEL_PATH + "drone.obj").c_str());
		T_drone.init(this, (TEXTURE_PATH + "droneTexUK3.png").c_str());
		DS_drone.init(this, &DSLobj, {
					{0, UNIFORM, sizeof(UniformBufferObject), nullptr},
					{1, TEXTURE, 0, &T_drone}
			});

		DS_global.init(this, &DSLglobal, {
					{0, UNIFORM, sizeof(globalUniformBufferObject), nullptr}
			});



		int width, height, nrChannels;
		// load height map
		unsigned char* hMap = stbi_load((TEXTURE_PATH + "terrain_height.png").c_str(), &width, &height, &nrChannels, 0);
		if (hMap)
		{
			std::cout << "Loaded heightmap of size " << height << " x " << width << std::endl;
		}
		else
		{
			std::cout << "Failed to load texture" << std::endl;
		}

		// yShift va solo a cambiare inizio/fine - è un offset
		// yScale invece aumenta/diminuisce i valori di Y. Più è vicino a uno e più valor ci sono
		float yScale = 256.0f / 256.0f, yShift = 0.0f;
		int rez = 1;
		unsigned bytePerPixel = nrChannels;
		for (int i = 0; i < height; i++)
		{
			for (int j = 0; j < width; j++)
			{
				unsigned char* pixelOffset = hMap + (i + width * j) * bytePerPixel;
				unsigned char y = pixelOffset[0];

				// vertex
				vertices.push_back( -height/2.0f + height*i/(float)height );   // vx
				vertices.push_back((int)y * yScale - yShift);   // vy
				vertices.push_back(-width / 2.0f + width * j / (float)width);   // vz
			}

		}
		// row order
		//std::cout << vertices[0] << "\t" << vertices[1] << "\t"<< vertices[2]<< "\n";
		//std::cout << vertices[3] << "\t" << vertices[4] << "\t" << vertices[5] << "\n";
			
	}

	// Here you destroy all the objects you created!		
	void localCleanup() {
		DS_ground.cleanup();
		T_ground.cleanup();
		M_ground.cleanup();

		DS_drone.cleanup();
		T_drone.cleanup();
		M_drone.cleanup();


		DS_global.cleanup();

		P1.cleanup();
		DSLglobal.cleanup();
		DSLobj.cleanup();
	}

	// Here it is the creation of the command buffer:
	// You send to the GPU all the objects you want to draw,
	// with their buffers and textures
	void populateCommandBuffer(VkCommandBuffer commandBuffer, int currentImage) {

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
			P1.graphicsPipeline);
		vkCmdBindDescriptorSets(commandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			P1.pipelineLayout, 0, 1, &DS_global.descriptorSets[currentImage],
			0, nullptr);


		VkBuffer vertexBuffers[] = { M_ground.vertexBuffer };
		// property .vertexBuffer of models, contains the VkBuffer handle to its vertex buffer
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
		// property .indexBuffer of models, contains the VkBuffer handle to its index buffer
		vkCmdBindIndexBuffer(commandBuffer, M_ground.indexBuffer, 0,
			VK_INDEX_TYPE_UINT32);

		// property .pipelineLayout of a pipeline contains its layout.
		// property .descriptorSets of a descriptor set contains its elements.
		vkCmdBindDescriptorSets(commandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			P1.pipelineLayout, 1, 1, &DS_ground.descriptorSets[currentImage],
			0, nullptr);

		// property .indices.size() of models, contains the number of triangles * 3 of the mesh.
		vkCmdDrawIndexed(commandBuffer,
			static_cast<uint32_t>(M_ground.indices.size()), 1, 0, 0, 0);


		VkBuffer vertexBuffers2[] = { M_drone.vertexBuffer };
		VkDeviceSize offsets2[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers2, offsets2);
		vkCmdBindIndexBuffer(commandBuffer, M_drone.indexBuffer, 0,
			VK_INDEX_TYPE_UINT32);
		vkCmdBindDescriptorSets(commandBuffer,
			VK_PIPELINE_BIND_POINT_GRAPHICS,
			P1.pipelineLayout, 1, 1, &DS_drone.descriptorSets[currentImage],
			0, nullptr);
		vkCmdDrawIndexed(commandBuffer,
			static_cast<uint32_t>(M_drone.indices.size()), 1, 0, 0, 0);


	}

	// Here is where you update the uniforms.
	// Very likely this will be where you will be writing the logic of your application.
	void updateUniformBuffer(uint32_t currentImage) {
		static auto startTime = std::chrono::high_resolution_clock::now();
		auto currentTime = std::chrono::high_resolution_clock::now();
		float time = std::chrono::duration<float, std::chrono::seconds::period>
			(currentTime - startTime).count();

		static float lastTime = 0.0f;
		float deltaT = time - lastTime;
		lastTime = time;

		//initial position of the drone
		static glm::vec3 currentPos = glm::vec3(0.0f, 0.0f, 0.0f);
		glm::vec3 oldPos = currentPos;

		const float rotSpeed = glm::radians(50.0f);
		const float MOVE_SPEED = 150.0f;
		float tempSpeed = MOVE_SPEED;
		//speed for upper & lower vertical movements
		const float vertSpeed = 5.0f;


		static float yaw = 0.0;
		static float pitch = 0.0;
		static float roll = 0.0;

		// rotation around the vertical axis of the drone
		static float rotY = 0.0f;;
		glm::mat4 rotateY(1.0f);

		// for speed increase
		static float count = 0;

		globalUniformBufferObject gubo{};
		UniformBufferObject ubo{};

		void* data;

		// increase speed
		if (glfwGetKey(window, GLFW_KEY_LEFT) || glfwGetKey(window, GLFW_KEY_RIGHT) ||
			glfwGetKey(window, GLFW_KEY_UP) || glfwGetKey(window, GLFW_KEY_DOWN)) {
			count = count + 0.1;
			if (count > 5) {
				tempSpeed += tempSpeed + 1;
			}
		}
		else {
			count = 0;
			// reset speed to original value
			tempSpeed = MOVE_SPEED;
		}


		if (glfwGetKey(window, GLFW_KEY_LEFT)) {
			currentPos -= tempSpeed * glm::vec3(glm::rotate(glm::mat4(1.0f), -yaw,
				glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(1, 0, 0, 1)) * deltaT;

			if (roll < glm::radians(20.0f)) {
				roll += deltaT * rotSpeed;
			}

		}
		// reset original orientation of the drone
		if (!glfwGetKey(window, GLFW_KEY_LEFT)) {
			if (roll > glm::radians(0.5f)) {
				roll -= deltaT * rotSpeed;
				//roll = 0.0f;
			}
		}

		if (glfwGetKey(window, GLFW_KEY_RIGHT)) {
			currentPos += tempSpeed * glm::vec3(glm::rotate(glm::mat4(1.0f), -yaw,
				glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(1, 0, 0, 1)) * deltaT;

			if (roll > -glm::radians(20.0f)) {
				roll -= deltaT * rotSpeed;
			}
		}
		// reset original orientation of the drone
		if (!glfwGetKey(window, GLFW_KEY_RIGHT)) {
			if (roll < glm::radians(0.5f)) {
				roll += deltaT * rotSpeed;
			}
		}

		if (glfwGetKey(window, GLFW_KEY_UP)) {
			currentPos -= tempSpeed * glm::vec3(glm::rotate(glm::mat4(1.0f), -yaw,
				glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(0, 0, 1, 1)) * deltaT;

			if (pitch < glm::radians(15.0f)) {
				pitch += deltaT * rotSpeed;
			}
		}
		// reset original orientation of the drone
		if (!glfwGetKey(window, GLFW_KEY_UP)) {
			if (pitch > glm::radians(0.5f)) {
				pitch -= deltaT * rotSpeed;
			}
		}

		if (glfwGetKey(window, GLFW_KEY_DOWN)) {
			currentPos += tempSpeed * glm::vec3(glm::rotate(glm::mat4(1.0f), -yaw,
				glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(0, 0, 1, 1)) * deltaT;

			if (pitch > -glm::radians(15.0f)) {
				pitch -= deltaT * rotSpeed;
			}
		}
		// reset original orientation of the drone
		if (!glfwGetKey(window, GLFW_KEY_DOWN)) {
			if (pitch < glm::radians(0.5f)) {
				pitch += deltaT * rotSpeed;
			}
		}

		// rotation around the y axis of the drone
		// counterclockwise
		if (glfwGetKey(window, GLFW_KEY_A)) {
			rotY -= 2.0f;
			rotateY = glm::rotate(rotateY, glm::radians(rotY), glm::vec3(0, 1, 0));
			currentPos += MOVE_SPEED * glm::vec3(glm::rotate(rotateY, glm::radians(-rotY), glm::vec3(0, 1, 0)) * glm::vec4(0, 0, 0, 1)) * deltaT;

		}

		// clockwise
		if (glfwGetKey(window, GLFW_KEY_D)) {

			rotY += 2.0f;
			rotateY = glm::rotate(rotateY, glm::radians(rotY), glm::vec3(0, 1, 0));
			currentPos += MOVE_SPEED * glm::vec3(glm::rotate(rotateY, glm::radians(-rotY), glm::vec3(0, 1, 0)) * glm::vec4(0, 0, 0, 1)) * deltaT;

		}


		//lower vertical movement
		if (glfwGetKey(window, GLFW_KEY_F)) {
			currentPos += glm::vec3(0, vertSpeed, 0);
		}
		//upper vertical movement
		if (glfwGetKey(window, GLFW_KEY_R)) {
			currentPos -= glm::vec3(0, vertSpeed, 0);
		}

		// rotation around the y axis of the drone + around the y axis of the map
		if (glfwGetKey(window, GLFW_KEY_Q)) {
			rotY -= 0.5f;
			rotateY = glm::rotate(rotateY, glm::radians(rotY), glm::vec3(0, 1, 0));
			currentPos += MOVE_SPEED * glm::vec3(glm::rotate(rotateY, glm::radians(-rotY), glm::vec3(0, 1, 0)) * glm::vec4(0, 0, 0, 1)) * deltaT;

			yaw -= deltaT * rotSpeed;
		}
		if (glfwGetKey(window, GLFW_KEY_E)) {
			rotY += 0.5f;
			rotateY = glm::rotate(rotateY, glm::radians(rotY), glm::vec3(0, 1, 0));
			currentPos += MOVE_SPEED * glm::vec3(glm::rotate(rotateY, glm::radians(-rotY), glm::vec3(0, 1, 0)) * glm::vec4(0, 0, 0, 1)) * deltaT;

			yaw += deltaT * rotSpeed;
		}

		// reset original setting
		if (glfwGetKey(window, GLFW_KEY_SPACE)) {
			yaw = 0.0;
			pitch = 0.0;
			roll = 0.0;
			currentPos.x = 0;
			currentPos.y = 0;
			currentPos.z = 0;
			rotY = 0;
		}

		// map boundaries
		// max Y: -127 approx --> max elevation: 500
		if (abs(currentPos.x) > xTexTerrain || abs(currentPos.z) > zTexTerrain || abs(currentPos.y) > 500) {
			currentPos.x = oldPos.x;
			currentPos.y = oldPos.y;
			currentPos.z = oldPos.z;
		}

		// check for collision
		if (findY(currentPos.x, currentPos.y, currentPos.z, 1024, 1018)) {
				currentPos.x = oldPos.x;
				currentPos.y = oldPos.y;
				currentPos.z = oldPos.z;
		}
		

		//// print map position
		//if (glfwGetKey(window, GLFW_KEY_O)) {
		//	for (int i = 0; i < 3; i++) {
		//		std::cout << currentPos[i] << "\t";
		//	}
		//	std::cout << "\n";
		//	
		//	bool test = findY(-1634.06, currentPos.y, -1620.86, 1024, 1018);

		//}

		// terrain
		ubo.model = glm::rotate(glm::mat4(1.0f), glm::radians(yaw * 0.1f), glm::vec3(0.0f, 1.0f, 0.0f)) * glm::translate(glm::mat4(1.0f), currentPos);

		vkMapMemory(device, DS_ground.uniformBuffersMemory[0][currentImage], 0,
			sizeof(ubo), 0, &data);
		memcpy(data, &ubo, sizeof(ubo));
		vkUnmapMemory(device, DS_ground.uniformBuffersMemory[0][currentImage]);


		// drone
		ubo.model = glm::rotate(glm::mat4(1.0f), -yaw, glm::vec3(0.0f, 1.0f, 0.0f))
			* glm::rotate(glm::mat4(1.0f), pitch, glm::vec3(1.0f, 0.0f, 0.0f))
			* glm::rotate(glm::mat4(1.0f), -roll, glm::vec3(0.0f, 0.0f, 1.0f))
			* glm::rotate(glm::mat4(1.0f), glm::radians(-rotY), glm::vec3(0, 1, 0));


		vkMapMemory(device, DS_drone.uniformBuffersMemory[0][currentImage], 0,
			sizeof(ubo), 0, &data);
		memcpy(data, &ubo, sizeof(ubo));
		vkUnmapMemory(device, DS_drone.uniformBuffersMemory[0][currentImage]);

		
		gubo.view = glm::lookAt(glm::vec3(sin(yaw), 0.5f, -cos(yaw)),
			glm::vec3(0.0f, 0.0f, 0.0f),	//position
			glm::vec3(0.0f, 1.0f, 0.0f));	// rot		
		gubo.proj = glm::perspective(glm::radians(110.0f),
			swapChainExtent.width / (float)swapChainExtent.height,
			0.1f, 1000.0f);

		gubo.proj[1][1] *= -1;


		vkMapMemory(device, DS_global.uniformBuffersMemory[0][currentImage], 0,
			sizeof(gubo), 0, &data);
		memcpy(data, &gubo, sizeof(gubo));
		vkUnmapMemory(device, DS_global.uniformBuffersMemory[0][currentImage]);

	}
};

// This is the main: probably you do not need to touch this!
int main() {
	MyProject app;

	try {
		app.run();
	}
	catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}