#include "UnitTests.h"

#include "../NULLC/nullc.h"
#include "../NULLC/nullc_debug.h"
#include "../NULLC/nullc_internal.h"

// Check that remote debug module compiles correctly
#if defined(_MSC_VER)
	#include "../NULLC/nullc_remote.h"
#endif

#include "../NULLC/includes/file.h"
#include "../NULLC/includes/math.h"
#include "../NULLC/includes/vector.h"
#include "../NULLC/includes/random.h"
#include "../NULLC/includes/dynamic.h"
#include "../NULLC/includes/gc.h"
#include "../NULLC/includes/time.h"
#include "../NULLC/includes/string.h"

#include "../NULLC/includes/canvas.h"
#include "../NULLC/includes/window.h"
#include "../NULLC/includes/io.h"

#include "../NULLC/includes/pugi.h"

#include "TestBase.h"
#include "TestSpeed.h"
#include "TestCompileFail.h"
#include "TestParseFail.h"
#include "TestInterface.h"

#if defined(_MSC_VER)
#pragma warning(disable: 4127 4996)
#endif

//#define ALLOC_TOP_DOWN
//#define NO_CUSTOM_ALLOCATOR

unsigned testTotalMemoryAlloc = 0;
unsigned testTotalMemoryFree = 0;
unsigned testTotalMemoryRequested = 0;
unsigned testTotalMemoryUsed = 0;

struct MallocAllocatorRef : Allocator
{
	MallocAllocatorRef()
	{
	}

	~MallocAllocatorRef()
	{
	}

	virtual void* alloc(int size)
	{
		return malloc(size);
	}

	virtual void dealloc(void* ptr)
	{
		free(ptr);
	}

	virtual unsigned requested()
	{
		return 0;
	}
};

struct PointerHasher
{
	unsigned operator()(void *key)
	{
		char data[sizeof(void*)];
		memcpy(data, &key, sizeof(void*));

		unsigned int hash = 5381;
		for(unsigned i = 0; i < sizeof(void*); i++)
			hash = ((hash << 5) + hash) + data[i];
		return hash;
	}
};

MallocAllocatorRef setAllocator;
SmallDenseMap<void*, bool, PointerHasher, 1024> activePoiners(&setAllocator);

void* testAlloc(int size)
{
	testTotalMemoryAlloc++;
	testTotalMemoryRequested += size;
	testTotalMemoryUsed += size;

#ifdef ALLOC_TOP_DOWN
	char *ptr = (char*)VirtualAlloc(NULL, size + 128, MEM_COMMIT | MEM_TOP_DOWN, PAGE_READWRITE);
#else
	char *ptr = new char[size + 128];
#endif

	if(size < 0 || !ptr)
	{
		assert(!"out of memory");
		return 0;
	}

	memset(ptr, 0xee, 128);
	*(unsigned*)ptr = size;

	activePoiners.insert(ptr, 1);

	return ptr + 128;
}

void testDealloc(void* ptr)
{
	if(!ptr)
		return;

	ptr = (char*)ptr - 128;

	bool* active = activePoiners.find(ptr);

	if(!active || !*active)
	{
		printf("pointer was not allocated");
		abort();
	}

	activePoiners.insert(ptr, 0);

	testTotalMemoryFree++;
	testTotalMemoryUsed -= *(unsigned*)ptr;

	for(unsigned i = sizeof(unsigned); i < 128; i++)
		assert(((unsigned char*)ptr)[i] == 0xee);

#ifdef ALLOC_TOP_DOWN
	VirtualFree((char*)ptr, 0, MEM_RELEASE);
#else
	delete[] (char*)ptr;
#endif
}

nullres CompileFile(const char* fileName)
{
	static char content[64 * 1024];

	FILE *euler = fopen(fileName, "rb");
	fseek(euler, 0, SEEK_END);
	unsigned int textSize = ftell(euler);
	assert(textSize < 64 * 1024);
	fseek(euler, 0, SEEK_SET);
	fread(content, 1, textSize, euler);
	content[textSize] = 0;
	fclose(euler);

	return nullcCompile(content);
}

int RunTests(bool verbose, const char* (*fileLoadFunc)(const char*, unsigned*), void (*fileFreeFunc)(const char*), bool runSpeedTests, bool testOutput, bool testTranslationSave, bool testTranslation)
{
	Tests::messageVerbose = verbose;
	Tests::fileLoadFunc = fileLoadFunc;
	Tests::fileFreeFunc = fileFreeFunc;

	// Extra tests

	// Safe sprintf test
	{
		testsCount[TEST_TYPE_EXTRA]++;

		char buf[8];
		char *pos = buf + NULLC::SafeSprintf(buf, 8, "this ");
		pos += NULLC::SafeSprintf(pos, 8 - int(pos - buf), "string is too long");
		if(memcmp(buf, "this st", 8) != 0)
			printf("Safe sprintf test failed: string is incorrect\n");
		else if(pos != buf + 8)
			printf("Safe sprintf test failed: iterator is incorrect\n");
		else
			testsPassed[TEST_TYPE_EXTRA]++;
	}

	{
		testsCount[TEST_TYPE_EXTRA]++;

		char buf[8];
		char *pos = buf + NULLC::SafeSprintf(buf, 8, "this string");
		pos += NULLC::SafeSprintf(pos, 8 - int(pos - buf), " is too long");
		if(memcmp(buf, "this st", 8) != 0)
			printf("Safe sprintf test failed: string is incorrect\n");
		else if(pos != buf + 8)
			printf("Safe sprintf test failed: iterator is incorrect\n");
		else
			testsPassed[TEST_TYPE_EXTRA]++;
	}

/*
	unsigned int tStart = clock();
	for(unsigned int i = 0; i < 10000; i++)
	{
		nullcInit("Modules/");
		nullcTerminate();
	}
	printf("Finished in %d\r\n", clock() - tStart);
*/

	Tests::enableLogFiles = testOutput;
	Tests::doSaveTranslation = testTranslationSave;
	Tests::doTranslation = testTranslation;

	if(testTranslation)
	{
		Tests::messageVerbose = true;
		Tests::openStreamFunc = 0;
		Tests::writeStreamFunc = 0;
		Tests::closeStreamFunc = 0;
	}

	// To test translation to C with build and execution
	/*
	Tests::messageVerbose = true;
	Tests::doSaveTranslation = true;
	Tests::doTranslation = true;
	Tests::openStreamFunc = 0;
	Tests::writeStreamFunc = 0;
	Tests::closeStreamFunc = 0;
	*/

	// To enable real log files during tests
	/*
	Tests::enableLogFiles = true;
	Tests::messageVerbose = true;
	Tests::openStreamFunc = 0;
	Tests::writeStreamFunc = 0;
	Tests::closeStreamFunc = 0;
	*/

	// Init NULLC for interface tests
#ifdef NO_CUSTOM_ALLOCATOR
	nullcInit();
	nullcAddImportPath(MODULE_PATH_A);
	nullcAddImportPath(MODULE_PATH_B);
#else
	nullcInitCustomAlloc(testAlloc, testDealloc);
	nullcAddImportPath(MODULE_PATH_A);
	nullcAddImportPath(MODULE_PATH_B);
#endif
	nullcSetFileReadHandler(Tests::fileLoadFunc, Tests::fileFreeFunc);
	nullcSetEnableLogFiles(Tests::enableLogFiles, Tests::openStreamFunc, Tests::writeStreamFunc, Tests::closeStreamFunc);

	nullcInitTypeinfoModule();
	nullcInitDynamicModule();
	RunInterfaceTests();

	// Init NULLC for test set
#ifdef NO_CUSTOM_ALLOCATOR
	nullcInit();
	nullcAddImportPath(MODULE_PATH_A);
	nullcAddImportPath(MODULE_PATH_B);
#else
	nullcInitCustomAlloc(testAlloc, testDealloc);
	nullcAddImportPath(MODULE_PATH_A);
	nullcAddImportPath(MODULE_PATH_B);
#endif
	nullcSetFileReadHandler(Tests::fileLoadFunc, Tests::fileFreeFunc);
	nullcSetEnableLogFiles(Tests::enableLogFiles, Tests::openStreamFunc, Tests::writeStreamFunc, Tests::closeStreamFunc);

	nullcInitTypeinfoModule();
	nullcInitFileModule();
	nullcInitMathModule();
	nullcInitVectorModule();
	nullcInitRandomModule();
	nullcInitDynamicModule();
	nullcInitGCModule();
	nullcInitStringModule();

	nullcInitIOModule();
	nullcInitCanvasModule();

#if defined(_MSC_VER)
	nullcInitWindowModule();
#endif

	/*
	//SpeedTestFile("test_document.nc");
	//SpeedTestFile("shapes.nc");
	//SpeedTestFile("raytrace.nc");
	//SpeedTestFile("blob.nc");
	return 0;*/

	RunParseFailTests();
	RunCompileFailTests();

	TestQueue queue;
	queue.RunTests();

	// Conclusion 
	printf("VM passed %d of %d tests\n", testsPassed[TEST_TYPE_VM], testsCount[TEST_TYPE_VM]);

	printf("Expr Evaluated %d of %d tests\n", testsPassed[TEST_TYPE_EXPR_EVALUATION], testsCount[TEST_TYPE_EXPR_EVALUATION]);
	printf("Inst Evaluated %d of %d tests\n", testsPassed[TEST_TYPE_INST_EVALUATION], testsCount[TEST_TYPE_INST_EVALUATION]);

#ifdef NULLC_BUILD_X86_JIT
	printf("X86 passed %d of %d tests\n", testsPassed[TEST_TYPE_X86], testsCount[TEST_TYPE_X86]);
#else
	testsPassed[TEST_TYPE_X86] = 0;
	testsCount[TEST_TYPE_X86] = 0;
#endif

#ifdef NULLC_LLVM_SUPPORT
	printf("LLVM passed %d of %d tests\n", testsPassed[TEST_TYPE_LLVM], testsCount[TEST_TYPE_LLVM]);
#else
	testsPassed[TEST_TYPE_LLVM] = 0;
	testsCount[TEST_TYPE_LLVM] = 0;
#endif

	printf("RegVM passed %d of %d tests\n", testsPassed[TEST_TYPE_REGVM], testsCount[TEST_TYPE_REGVM]);

	printf("Failure tests: passed %d of %d tests\n", testsPassed[TEST_TYPE_FAILURE], testsCount[TEST_TYPE_FAILURE]);
	printf("Extra tests: passed %d of %d tests\n", testsPassed[TEST_TYPE_EXTRA], testsCount[TEST_TYPE_EXTRA]);
	printf("Translation tests: passed %d of %d tests\n", testsPassed[TEST_TYPE_TRANSLATION], testsCount[TEST_TYPE_TRANSLATION]);

	unsigned allTests = 0;
	unsigned allPassed = 0;
	for(unsigned i = 0; i < TEST_TYPE_COUNT; i++)
	{
		allTests += testsCount[i];
		allPassed += testsPassed[i];
	}

	printf("Compilation time: %f\n", Tests::timeCompile);
	printf("Get bytecode time: %f\n", Tests::timeGetBytecode);
	printf("Tree visit time: %f\n", Tests::timeVisit);
	printf("Expression evaluation time: %f\n", Tests::timeExprEvaluate);
	printf("Instruction evaluation time: %f\n", Tests::timeInstEvaluate);
	printf("Translation time: %f\n", Tests::timeTranslate);
	printf("Clean time: %f\n", Tests::timeClean);
	printf("Link time: %f\n", Tests::timeLinkCode);
	printf("Run time: %f\n", Tests::timeRun);

	printf("Total time: %f\n", Tests::timeCompile + Tests::timeGetBytecode + Tests::timeVisit + Tests::timeExprEvaluate + Tests::timeInstEvaluate + Tests::timeTranslate + Tests::timeClean + Tests::timeLinkCode + Tests::timeRun);

	printf("Total log output: %lld\n", Tests::totalOutput);
	printf("Total nodes: %d syntax, %d expression\n", Tests::totalSyntaxNodes, Tests::totalExpressionNodes);

	printf("Total global allocs: %d (%.3fMB requested)\n", testTotalMemoryAlloc, testTotalMemoryRequested / 1024 / 1024.0);

	printf("Passed %d of %d tests\n", allPassed, allTests);

	if(runSpeedTests)
		RunSpeedTests();

	// Terminate NULLC
	nullcTerminate();

	return allPassed == allTests ? 0 : 1;
}
