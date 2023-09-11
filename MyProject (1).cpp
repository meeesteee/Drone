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

// terrain height map
stbi_uc* texTerrain;
int texTerrainWidth, texTerrainHeight;	// number of pixels (1018x1024)

// height map max values
const float xTexTerrain = 1661;
const float zTexTerrain = 1661;


//returns the height of a specific point xyz on the terrain
//it will be used to check if the height of object collides 
// trova il punto del drone corrispondente nella mappa e ne confronto le altezze
int collide(float x, float y, float z) {

	int pixX = round(fmax(0.0f, fmin(texTerrainWidth - 1, (-x + 1661) / (3322 / texTerrainWidth) - 40))); //-1660 to +1660
	int pixZ = round(fmax(0.0f, fmin(texTerrainHeight - 1, (-z + 1661) / (3322 / texTerrainHeight) - 40)));//-1660 to +1660

	int pix = (int)texTerrain[texTerrainWidth * pixZ + pixX];//1D arrayi 2D tarama
	
	pix = pix + y;//pix height
	//std::cout << "pixX: " << pixX << " pixZ: " << pixZ << " pix:" << pix << "\n";
	return pix;
}
//end of collision



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


		//load texture heigth map
		texTerrain = stbi_load((TEXTURE_PATH + "terrain_height.png").c_str(),
			&texTerrainWidth, &texTerrainHeight,
			NULL, 1);
		std::cout << texTerrainHeight << "\t" <<texTerrainWidth;

		// check texture open
		if (!texTerrain) {
			std::cout << (TEXTURE_PATH + "terrain_height.png").c_str() << "\n";
			throw std::runtime_error("failed to load texture image for the map!");
		}
		
	
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

		const float ROT_SPEED = glm::radians(50.0f);
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


		glm::vec3 oldPos = currentPos;

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
		else if (glfwGetKey(window, GLFW_KEY_F) || glfwGetKey(window, GLFW_KEY_R)) {
			count = count + 0.1;
			if (count > 1) {
				tempSpeed += tempSpeed + 1;
			}
		
		} else {
			count = 0;
			tempSpeed = MOVE_SPEED;
		}



		if (glfwGetKey(window, GLFW_KEY_LEFT)) {
			currentPos -= tempSpeed * glm::vec3(glm::rotate(glm::mat4(1.0f), -yaw,
				glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(1, 0, 0, 1)) * deltaT;

			if (roll < glm::radians(20.0f)) {
				roll += deltaT * ROT_SPEED;
			}

		}
		// reset original orientation of the drone
		if (!glfwGetKey(window, GLFW_KEY_LEFT)) {
			if (roll > glm::radians(0.5f)) {
				roll -= deltaT * ROT_SPEED;
				//roll = 0.0f;
			}
		}

		if (glfwGetKey(window, GLFW_KEY_RIGHT)) {
			currentPos += tempSpeed * glm::vec3(glm::rotate(glm::mat4(1.0f), -yaw,
				glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(1, 0, 0, 1)) * deltaT;

			if (roll > -glm::radians(20.0f)) {
				roll -= deltaT * ROT_SPEED;
			}
		}
		// reset original orientation of the drone
		if (!glfwGetKey(window, GLFW_KEY_RIGHT)) {
			if (roll < glm::radians(0.5f)) {
				roll += deltaT * ROT_SPEED;
			}
		}

		if (glfwGetKey(window, GLFW_KEY_UP)) {
			currentPos -= tempSpeed * glm::vec3(glm::rotate(glm::mat4(1.0f), -yaw,
				glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(0, 0, 1, 1)) * deltaT;

			if (pitch < glm::radians(15.0f)) {
				pitch += deltaT * ROT_SPEED;
			}
		}
		// reset original orientation of the drone
		if (!glfwGetKey(window, GLFW_KEY_UP)) {
			if (pitch > glm::radians(0.5f)) {
				pitch -= deltaT * ROT_SPEED;
			}
		}

		if (glfwGetKey(window, GLFW_KEY_DOWN)) {
			currentPos += tempSpeed * glm::vec3(glm::rotate(glm::mat4(1.0f), -yaw,
				glm::vec3(0.0f, 1.0f, 0.0f)) * glm::vec4(0, 0, 1, 1)) * deltaT;

			if (pitch > -glm::radians(15.0f)) {
				pitch -= deltaT * ROT_SPEED;
			}
		}
		// reset original orientation of the drone
		if (!glfwGetKey(window, GLFW_KEY_DOWN)) {
			if (pitch < glm::radians(0.5f)) {
				pitch += deltaT * ROT_SPEED;
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

			yaw -= deltaT * ROT_SPEED;
		}
		if (glfwGetKey(window, GLFW_KEY_E)) {
			rotY += 0.5f;
			rotateY = glm::rotate(rotateY, glm::radians(rotY), glm::vec3(0, 1, 0));
			currentPos += MOVE_SPEED * glm::vec3(glm::rotate(rotateY, glm::radians(-rotY), glm::vec3(0, 1, 0)) * glm::vec4(0, 0, 0, 1)) * deltaT;

			yaw += deltaT * ROT_SPEED;
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


		// collision
		int col_thresh = 95;
		if (collide(currentPos.x,currentPos.y,currentPos.z)> col_thresh) {
			currentPos.x = oldPos.x;
			currentPos.y = oldPos.y;
			currentPos.z = oldPos.z;
		}

		// print map position
		if (glfwGetKey(window, GLFW_KEY_O)) {
			for (int i = 0; i < 3; i++) {
				std::cout << currentPos[i] << "\t";
			}
			std::cout << "\n";
			std::cout << collide(currentPos.x, currentPos.y, currentPos.z) << "\n";
			std::cout << "\n";
		}
	
		// terrain
		ubo.model = glm::rotate(glm::mat4(1.0f), glm::radians(yaw * 0.1f), glm::vec3(0.0f, 1.0f, 0.0f)) * glm::translate(glm::mat4(1.0f), glm::vec3(-1630.682373, 101.007027, 1661.160034));
		
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

		//global view
		gubo.view = glm::lookAt(glm::vec3(sin(yaw), 0.6f, -cos(yaw)),
			glm::vec3(0.0f, 0.0f, 0.0f),
			glm::vec3(0.0f, 1.0f, 0.0f));
		//perspective projection with 45 degree field of view (fovy), 0.1 near plane, 1000 far plane
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