#include "test_suite.h"

#include "../../SharedDir/opcodes.h"
#include "../../LibAPie/common/file.h"
#include "test_server.h"

namespace APie {
	TestSuite::TestSuite(uint64_t iRoleId) :
		m_roleId(iRoleId)
	{
		m_ptrRole = MockRole::createMockRole(iRoleId);
	}

	bool TestSuite::init()
	{
		bool bResult = loadConfig();
		m_init = bResult;

		return bResult;
	}

	uint64_t TestSuite::getRoleId()
	{
		return m_roleId;
	}

	void TestSuite::setUp()
	{
		m_ptrRole->start();
		m_ptrRole->disableCmdTimer();

		TestServerMgrSingleton::get().addMockRole(m_ptrRole);
	}

	void TestSuite::tearDown()
	{

	}

	bool TestSuite::isComplete()
	{
		uint32_t iSize = m_taskList.size();

		if (m_runTaskIndex < iSize)
		{
			return false;
		}
		else
		{
			return true;
		}
	}

	void TestSuite::clear()
	{
		m_taskList.clear();
		m_ptrRole.reset();
	}

	Json::Value TestSuite::getReport()
	{
		Json::Value root;
		root["roleId"] = m_roleId;

		if (m_ptrRole)
		{
			Json::Value delay;
			for (auto& elems : m_ptrRole->getReplyDelay())
			{
				uint32_t iK1 = std::get<0>(elems.first);
				uint32_t iK2 = std::get<1>(elems.first);
				std::string sKey = std::to_string(iK1) + "->" + std::to_string(iK2);

				uint64_t iMin = 0;
				uint64_t iMax = 0;
				uint64_t iAverage = 0;
				uint64_t iTotal = 0;
				uint64_t iCount = 0;
				for (auto& items : elems.second)
				{
					if (iMin == 0)
					{
						iMin = items;
					}

					if (iMax == 0)
					{
						iMax = items;
					}

					if (iMin > items)
					{
						iMin = items;
					}

					if (iMax < items)
					{
						iMax = items;
					}

					iCount++;

					iTotal += items;
				}


				auto& mergeDelay = m_ptrRole->getMergeReplyDelay();
				auto findIte = mergeDelay.find(elems.first);
				if (findIte != mergeDelay.end())
				{
					if (std::get<0>(findIte->second) < iMin)
					{
						iMin = std::get<0>(findIte->second);
					}

					if (std::get<1>(findIte->second) > iMax)
					{
						iMax = std::get<1>(findIte->second);
					}

					iCount += std::get<2>(findIte->second);
					iTotal += std::get<3>(findIte->second);
				}

				Json::Value delayElem;
				delayElem["min"] = iMin;
				delayElem["max"] = iMax;
				delayElem["count"] = iCount;

				if (iCount != 0)
				{
					iAverage = iTotal / iCount;
				}
				delayElem["average"] = iAverage;

				delay[sKey] = delayElem;
			}


			root["delay"] = delay;
		}

		Json::Value summary;
		for (auto& elems : m_taskList)
		{
			Json::Value typeSummary;
			typeSummary["type"] = elems->getType();
			typeSummary["count"] = elems->getCount();
			typeSummary["successCount"] = elems->getSuccessCount();
			typeSummary["failureCount"] = elems->getFailureCount();

			std::string sType = std::to_string(elems->getType());
			summary[sType] = typeSummary;
		}

		root["task_summary"] = summary;


		return root;
	}

	void TestSuite::run()
	{
		if (!m_init)
		{
			return;
		}

		uint32_t iSize = m_taskList.size();
		if (m_runTaskIndex < iSize)
		{
			do 
			{
				auto iCurMs = Ctx::getCurMilliseconds();
				if (m_taskList[m_runTaskIndex]->hasTimeout(iCurMs))
				{
					m_taskList[m_runTaskIndex]->tearDown();
					m_taskList[m_runTaskIndex]->addCompleteLoop();

					m_taskList[m_runTaskIndex]->addFailureCount();
					m_taskList[m_runTaskIndex]->setStatus(ETestCaseStatus::ECS_INVALID);

					if (m_taskList[m_runTaskIndex]->isComplete())
					{
						m_runTaskIndex++;
					}
					break;
				}


				switch (m_taskList[m_runTaskIndex]->getStatus())
				{
				case ETestCaseStatus::ECS_INVALID:
				{
					if (!m_taskList[m_runTaskIndex]->expiredLoopTime(iCurMs))
					{
						break;
					}

					m_taskList[m_runTaskIndex]->setUp();
					m_taskList[m_runTaskIndex]->setStatus(ETestCaseStatus::ECS_RUNNING);
					break;
				}
				case ETestCaseStatus::ECS_RUNNING:
				{
					m_taskList[m_runTaskIndex]->run();
					break;
				}
				case ETestCaseStatus::ECS_SUCCESS:
				case ETestCaseStatus::ECS_FAILURE:
				{
					m_taskList[m_runTaskIndex]->tearDown();
					m_taskList[m_runTaskIndex]->getRole().clearMsg();
					m_taskList[m_runTaskIndex]->addCompleteLoop();

					if (m_taskList[m_runTaskIndex]->getStatus() == ETestCaseStatus::ECS_SUCCESS)
					{
						m_taskList[m_runTaskIndex]->addSuccessCount();
					}
					else
					{
						m_taskList[m_runTaskIndex]->addFailureCount();
					}
					m_taskList[m_runTaskIndex]->setStatus(ETestCaseStatus::ECS_INVALID);

					if (m_taskList[m_runTaskIndex]->isComplete())
					{
						m_runTaskIndex++;
					}

					break;
				}
				default:
				{
					break;
				}
				}
			} while (false);
		}
	}

	bool TestSuite::loadConfig()
	{
		auto nodeObj = APie::CtxSingleton::get().yamlAs<YAML::Node>({ "auto_test", "task_suite" }, YAML::Node());

		for (const auto& elems : nodeObj)
		{
			auto iType = APie::CtxSingleton::get().nodeYamlAs<uint32_t>(elems, { "task_case", "case_type" }, 0);
			auto iCount = APie::CtxSingleton::get().nodeYamlAs<uint32_t>(elems, { "task_case", "loop_count" }, 1);
			auto iInterval = APie::CtxSingleton::get().nodeYamlAs<uint32_t>(elems, { "task_case", "loop_interval_ms" }, 10);


			auto ptrCase = TestCaseFactory::create(*m_ptrRole, iType);
			if (ptrCase == nullptr)
			{
				return false;
			}
			ptrCase->init(iCount, iInterval);
			m_taskList.push_back(ptrCase);
		}

		return true;
	}

	std::shared_ptr<TestSuite> TestSuite::makeTestSuite(uint64_t iRoleId)
	{
		auto prtSuite = std::make_shared<TestSuite>(iRoleId);
		bool bResult = prtSuite->init();
		if (bResult)
		{
			return prtSuite;
		}
		else
		{
			return nullptr;
		}
	}
}

