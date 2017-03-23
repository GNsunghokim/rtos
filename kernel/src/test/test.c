#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "AceUnitData.h"
#include "test.h"

/* 
 * The number of test cases contained in this suite.
 * This is used to verify that the number of executed test cases is correct.
 */
#define TEST_CASES_FOR_VERIFICATION 4

int run_test(const char* name) {
	int ret = 0;
	// Run all tests for NULL parameter
	if(name == NULL) {
		// We suppose that there is only one suite
		runSuite(&suite1);

		// Verify all test were executed
		if(runnerData->testCaseCount != TEST_CASES_FOR_VERIFICATION) {
			printf("Test Cases: %d but expected %d\n", (int) runnerData->testCaseCount, TEST_CASES_FOR_VERIFICATION);
			ret = -1;
		}

	} else  {
		bool found = false;
		const TestSuite_t *const *suites = suite1.suites;
		if (suites != NULL) {
			for(; NULL != *suites; suites++) {
				// Find specific test function name from test fixtures
				TestFixture_t* fixture = (TestFixture_t*)*suites;

				if(!strncmp(fixture->name, name, strlen(name))) {
					runFixture(fixture, NULL);
					found = true;
					break;
				} 
			}
		}

		if(!found) {
			printf("Cannot find requested test : [%s]\n", name);
			return -2;
		}
	}

	if(runnerData->testCaseFailureCount != 0) 
		ret = -3;

	printf("Test Cases: %d  Success: %d  Errors: %d\n", (int)runnerData->testCaseCount,
		       	(int)runnerData->testCaseCount - runnerData->testCaseFailureCount,
			(int)runnerData->testCaseFailureCount);

	runnerData->testCaseCount = 0;
	runnerData->testCaseFailureCount = 0;

	return ret;
}

void list_tests() {
	const TestSuite_t *const *suites = suite1.suites;
	if(suites != NULL) {
		for(; NULL != *suites; suites++) {
			TestFixture_t* fixture = (TestFixture_t*)*suites;
			printf("%s\n", fixture->name);

			const testMethod_t* test = fixture->testCase;
			for(int i = 0; NULL != *test; i++, test++) {
				printf("\t%s\n", fixture->testNames[i]);
			}
		}
	}
}
