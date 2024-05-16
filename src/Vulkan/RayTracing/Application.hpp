#pragma once

#include "Vulkan/Application.hpp"
#include "RayTracingProperties.hpp"

namespace Vulkan
{
	class CommandBuffers;
	class Buffer;
	class DeviceMemory;
	class Image;
	class ImageView;
}

namespace Vulkan::RayTracing
{
	class Application : public Vulkan::Application
	{
	public:

		VULKAN_NON_COPIABLE(Application);

		void SetSupportRayTracing(bool supportRayTracing) { supportRayTracing_ = supportRayTracing; }

	protected:

		Application(const WindowConfig& windowConfig, VkPresentModeKHR presentMode, bool enableValidationLayers);
		~Application();

		void SetPhysicalDeviceImpl(VkPhysicalDevice physicalDevice,
			std::vector<const char*>& requiredExtensions,
			VkPhysicalDeviceFeatures& deviceFeatures,
			void* nextDeviceFeatures) override;
		
		void OnDeviceSet() override;
		void CreateAccelerationStructures();
		void DeleteAccelerationStructures();
		void CreateSwapChain() override;
		void DeleteSwapChain() override;
		void Render(VkCommandBuffer commandBuffer, uint32_t imageIndex) override;

		bool supportRayTracing_;
	private:

		void CreateBottomLevelStructures(VkCommandBuffer commandBuffer);
		void CreateTopLevelStructures(VkCommandBuffer commandBuffer);
		void CreateOutputImage();

		std::unique_ptr<class DeviceProcedures> deviceProcedures_;
		std::unique_ptr<class RayTracingProperties> rayTracingProperties_;

		std::vector<class BottomLevelAccelerationStructure> bottomAs_;
		std::unique_ptr<Buffer> bottomBuffer_;
		std::unique_ptr<DeviceMemory> bottomBufferMemory_;
		std::unique_ptr<Buffer> bottomScratchBuffer_;
		std::unique_ptr<DeviceMemory> bottomScratchBufferMemory_;
		std::vector<class TopLevelAccelerationStructure> topAs_;
		std::unique_ptr<Buffer> topBuffer_;
		std::unique_ptr<DeviceMemory> topBufferMemory_;
		std::unique_ptr<Buffer> topScratchBuffer_;
		std::unique_ptr<DeviceMemory> topScratchBufferMemory_;
		std::unique_ptr<Buffer> instancesBuffer_;
		std::unique_ptr<DeviceMemory> instancesBufferMemory_;

		std::unique_ptr<Image> accumulationImage_;
		std::unique_ptr<DeviceMemory> accumulationImageMemory_;
		std::unique_ptr<ImageView> accumulationImageView_;

		std::unique_ptr<Image> outputImage_;
		std::unique_ptr<DeviceMemory> outputImageMemory_;
		std::unique_ptr<ImageView> outputImageView_;

		std::unique_ptr<Image> pingpongImage0_;
		std::unique_ptr<DeviceMemory> pingpongImage0Memory_;
		std::unique_ptr<ImageView> pingpongImage0View_;

		std::unique_ptr<Image> pingpongImage1_;
		std::unique_ptr<DeviceMemory> pingpongImage1Memory_;
		std::unique_ptr<ImageView> pingpongImage1View_;
		
		std::unique_ptr<Image> gbufferImage_;
		std::unique_ptr<DeviceMemory> gbufferImageMemory_;
		std::unique_ptr<ImageView> gbufferImageView_;

		std::unique_ptr<Image> albedoImage_;
		std::unique_ptr<DeviceMemory> albedoImageMemory_;
		std::unique_ptr<ImageView> albedoImageView_;
		
		std::unique_ptr<class RayTracingPipeline> rayTracingPipeline_;
		std::unique_ptr<class DenoiserPipeline> denoiserPipeline_;
		std::unique_ptr<class ComposePipeline> composePipeline_;
		std::unique_ptr<class ShaderBindingTable> shaderBindingTable_;

		
	};

}
