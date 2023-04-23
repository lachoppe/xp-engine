#pragma once

#ifdef UNICODE
#define VK_CHECK(x)																		\
	do																					\
	{																					\
		VkResult err = x;																\
		if (err)																		\
		{																				\
			OutputMessage(L"[%d] Detected Vulkan error: %d\n", __LINE__, err);			\
			abort();																	\
		}																				\
	} while (0);
#else
#define VK_CHECK(x)																		\
	do																					\
	{																					\
		VkResult err = x;																\
		if (err)																		\
		{																				\
			OutputMessage("[%d] Detected Vulkan error: %d\n", __LINE__, err);			\
			abort();																	\
		}																				\
	} while (0);
#endif

void OutputMessage(const char* format, ...);
void OutputMessage(const wchar_t* format, ...);

#define ASSERT_M( condition, message, ... ) \
	if (!(condition)) { OutputMessage("\n\nASSERT FAILED at %s:%d:   " message "\n\n\n", __FILE__, __LINE__, __VA_ARGS__); abort(); }

#define ASSERT( condition ) \
	if (!(condition)) { OutputMessage("\n\nASSERT FAILED at %s:%d:   " #condition "\n\n\n", __FILE__, __LINE__); abort(); }

#define WARN_M( condition, message, ... ) \
	if (!(condition)) { OutputMessage("\nWARNING TEST FAILED at %s:%d:   " message "\n\n", __FILE__, __LINE__, __VA_ARGS__); }

#define WARN( condition ) \
	if (!(condition)) { OutputMessage("\nWARNING TEST FAILED at %s:%d:   " #condition "\n\n", __FILE__, __LINE__); }

#define DEBUG_M( message, ... ) \
	OutputMessage("\DEBUG MESSAGE at %s:%d:   " message "\n\n", __FILE__, __LINE__, __VA_ARGS__);
