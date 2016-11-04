// Copyright 2001-2016 Crytek GmbH / Crytek Group. All rights reserved.

#include "StdAfx.h"
#include "QueryHistory.h"
#include <CryRenderer/IRenderAuxGeom.h>

// *INDENT-OFF* - <hard to read code and declarations due to inconsistent indentation>

// make the global Serialize() functions available for use in yasli serialization
using uqs::core::Serialize;

namespace uqs
{
	namespace core
	{

		//===================================================================================
		//
		// CHistoricQuery: macros for enum serialization
		//
		//===================================================================================

		SERIALIZATION_ENUM_BEGIN_NESTED(CHistoricQuery, EQueryLifetimeStatus, "")
		SERIALIZATION_ENUM(CHistoricQuery::EQueryLifetimeStatus::QueryIsNotCreatedYet, "QueryIsNotCreatedYet", "")
		SERIALIZATION_ENUM(CHistoricQuery::EQueryLifetimeStatus::QueryIsAlive, "QueryIsAlive", "")
		SERIALIZATION_ENUM(CHistoricQuery::EQueryLifetimeStatus::QueryIsDestroyed, "QueryIsDestroyed", "")
		SERIALIZATION_ENUM_END()

		SERIALIZATION_ENUM_BEGIN_NESTED2(CHistoricQuery, SHistoricInstantEvaluatorResult, EStatus, "")
		SERIALIZATION_ENUM(CHistoricQuery::SHistoricInstantEvaluatorResult::EStatus::HasNotRunYet, "HasNotRunYet", "")
		SERIALIZATION_ENUM(CHistoricQuery::SHistoricInstantEvaluatorResult::EStatus::HasFinishedAndDiscardedTheItem, "HasFinishedAndDiscardedTheItem", "")
		SERIALIZATION_ENUM(CHistoricQuery::SHistoricInstantEvaluatorResult::EStatus::HasFinishedAndScoredTheItem, "HasFinishedAndScoredTheItem", "")
		SERIALIZATION_ENUM(CHistoricQuery::SHistoricInstantEvaluatorResult::EStatus::ExceptionOccurredInFunctionCall, "ExceptionOccurredInFunctionCall", "")
		SERIALIZATION_ENUM(CHistoricQuery::SHistoricInstantEvaluatorResult::EStatus::ExceptionOccurredInHimself, "ExceptionOccurredInHimself", "")
		SERIALIZATION_ENUM_END()

		SERIALIZATION_ENUM_BEGIN_NESTED2(CHistoricQuery, SHistoricDeferredEvaluatorResult, EStatus, "")
		SERIALIZATION_ENUM(CHistoricQuery::SHistoricDeferredEvaluatorResult::EStatus::HasNotRunYet, "HasNotRunYet", "")
		SERIALIZATION_ENUM(CHistoricQuery::SHistoricDeferredEvaluatorResult::EStatus::IsRunningNow, "IsRunningNow", "")
		SERIALIZATION_ENUM(CHistoricQuery::SHistoricDeferredEvaluatorResult::EStatus::HasFinishedAndDiscardedTheItem, "HasFinishedAndDiscardedTheItem", "")
		SERIALIZATION_ENUM(CHistoricQuery::SHistoricDeferredEvaluatorResult::EStatus::HasFinishedAndScoredTheItem, "HasFinishedAndScoredTheItem", "")
		SERIALIZATION_ENUM(CHistoricQuery::SHistoricDeferredEvaluatorResult::EStatus::GotAborted, "GotAborted", "")
		SERIALIZATION_ENUM(CHistoricQuery::SHistoricDeferredEvaluatorResult::EStatus::ExceptionOccurredInFunctionCall, "ExceptionOccurredInFunctionCall", "")
		SERIALIZATION_ENUM(CHistoricQuery::SHistoricDeferredEvaluatorResult::EStatus::ExceptionOccurredInHimself, "ExceptionOccurredInHimself", "")
		SERIALIZATION_ENUM_END()

		//===================================================================================
		//
		// CHistoricQuery::SHistoricInstantEvaluatorResult
		//
		//===================================================================================

		CHistoricQuery::SHistoricInstantEvaluatorResult::SHistoricInstantEvaluatorResult()
			: status(EStatus::HasNotRunYet)
			, nonWeightedScore(0.0f)
			, furtherInformationAboutStatus()
		{}

		void CHistoricQuery::SHistoricInstantEvaluatorResult::Serialize(Serialization::IArchive& ar)
		{
			ar(status, "status");
			ar(nonWeightedScore, "nonWeightedScore");
		}

		//===================================================================================
		//
		// CHistoricQuery::SHistoricDeferredEvaluatorResult
		//
		//===================================================================================

		CHistoricQuery::SHistoricDeferredEvaluatorResult::SHistoricDeferredEvaluatorResult()
			: status(EStatus::HasNotRunYet)
			, nonWeightedScore(0.0f)
			, furtherInformationAboutStatus()
		{}

		void CHistoricQuery::SHistoricDeferredEvaluatorResult::Serialize(Serialization::IArchive& ar)
		{
			ar(status, "status");
			ar(nonWeightedScore, "nonWeightedScore");
		}

		//===================================================================================
		//
		// CHistoricQuery::SHistoricItem
		//
		//===================================================================================

		CHistoricQuery::SHistoricItem::SHistoricItem()
			: accumulatedAndWeightedScoreSoFar(0.0f)
			, bDisqualifiedDueToBadScore(false)
		{}

		CHistoricQuery::SHistoricItem::SHistoricItem(const SHistoricItem& other)
			: resultOfAllInstantEvaluators(other.resultOfAllInstantEvaluators)
			, resultOfAllDeferredEvaluators(other.resultOfAllDeferredEvaluators)
			, accumulatedAndWeightedScoreSoFar(other.accumulatedAndWeightedScoreSoFar)
			, bDisqualifiedDueToBadScore(other.bDisqualifiedDueToBadScore)
			, pDebugProxy()	// bypass the unique_ptr
		{}

		CHistoricQuery::SHistoricItem& CHistoricQuery::SHistoricItem::operator=(const SHistoricItem& other)
		{
			if (this != &other)
			{
				resultOfAllInstantEvaluators = other.resultOfAllInstantEvaluators;
				resultOfAllDeferredEvaluators = other.resultOfAllDeferredEvaluators;
				accumulatedAndWeightedScoreSoFar = other.accumulatedAndWeightedScoreSoFar;
				bDisqualifiedDueToBadScore = other.bDisqualifiedDueToBadScore;
				// omit .pDebugProxy to bypass the unique_ptr
			}
			return *this;
		}

		void CHistoricQuery::SHistoricItem::Serialize(Serialization::IArchive& ar)
		{
			ar(resultOfAllInstantEvaluators, "resultOfAllInstantEvaluators");
			ar(resultOfAllDeferredEvaluators, "resultOfAllDeferredEvaluators");
			ar(accumulatedAndWeightedScoreSoFar, "accumulatedAndWeightedScoreSoFar");
			ar(bDisqualifiedDueToBadScore, "bDisqualifiedDueToBadScore");
			ar(pDebugProxy, "pDebugProxy");
		}

		//===================================================================================
		//
		// CHistoricQuery
		//
		//===================================================================================

		CHistoricQuery::CHistoricQuery()
			: m_pOwningHistoryManager(nullptr)
			, m_queryID(CQueryID::CreateInvalid())
			, m_parentQueryID(CQueryID::CreateInvalid())
			, m_queryLifetimeStatus(EQueryLifetimeStatus::QueryIsNotCreatedYet)
			, m_bGotCanceledPrematurely(false)
			, m_bExceptionOccurred(false)
			, m_longestEvaluatorName(0)
		{
			// nothing
		}

		CHistoricQuery::CHistoricQuery(const CQueryID& queryID, const char* querierName, const CQueryID& parentQueryID, CQueryHistoryManager* pOwningHistoryManager)
			: m_pOwningHistoryManager(pOwningHistoryManager)
			, m_queryID(queryID)
			, m_parentQueryID(parentQueryID)
			, m_querierName(querierName)
			, m_queryLifetimeStatus(EQueryLifetimeStatus::QueryIsNotCreatedYet)
			, m_bGotCanceledPrematurely(false)
			, m_bExceptionOccurred(false)
			, m_longestEvaluatorName(0)
		{
			// nothing
		}

		CDebugRenderWorld& CHistoricQuery::GetDebugRenderWorld()
		{
			return m_debugRenderWorld;
		}

		void CHistoricQuery::OnQueryCreated()
		{
			m_queryCreatedTimestamp = gEnv->pTimer->GetAsyncTime();
			m_queryLifetimeStatus = EQueryLifetimeStatus::QueryIsAlive;
		}

		void CHistoricQuery::OnQueryBlueprintInstantiationStarted(const char* queryBlueprintName)
		{
			m_queryBlueprintName = queryBlueprintName;
		}

		void CHistoricQuery::OnQueryCanceled(const CQueryBase::SStatistics& finalStatistics)
		{
			m_bGotCanceledPrematurely = true;
			m_finalStatistics = finalStatistics;
		}

		void CHistoricQuery::OnQueryFinished(const CQueryBase::SStatistics& finalStatistics)
		{
			m_finalStatistics = finalStatistics;
		}

		void CHistoricQuery::OnQueryDestroyed()
		{
			m_queryDestroyedTimestamp = gEnv->pTimer->GetAsyncTime();
			m_queryLifetimeStatus = EQueryLifetimeStatus::QueryIsDestroyed;

			// notify the top-level query-history-manager that the underlying query has now been finished
			m_pOwningHistoryManager->UnderlyingQueryIsGettingDestroyed(m_queryID);
		}

		void CHistoricQuery::OnExceptionOccurred(const char* exceptionMessage, const CQueryBase::SStatistics& finalStatistics)
		{
			m_exceptionMessage = exceptionMessage;
			m_bExceptionOccurred = true;
			m_finalStatistics = finalStatistics;
		}

		void CHistoricQuery::OnGenerationPhaseFinished(size_t numGeneratedItems, const CQueryBlueprint& queryBlueprint)
		{
			//
			// make room for all freshly generated items to record the outcome of all of their evaluators from now on
			//

			const std::vector<CInstantEvaluatorBlueprint*>& instantEvaluatorBlueprints = queryBlueprint.GetInstantEvaluatorBlueprints();
			const std::vector<CDeferredEvaluatorBlueprint*>& deferredEvaluatorBlueprints = queryBlueprint.GetDeferredEvaluatorBlueprints();

			const size_t numInstantEvaluatorBlueprints = instantEvaluatorBlueprints.size();
			const size_t numDeferredEvaluatorBlueprints = deferredEvaluatorBlueprints.size();

			// items
			m_items.resize(numGeneratedItems);
			for (SHistoricItem& item : m_items)
			{
				item.resultOfAllInstantEvaluators.resize(numInstantEvaluatorBlueprints);
				item.resultOfAllDeferredEvaluators.resize(numDeferredEvaluatorBlueprints);
			}

			// instant-evaluator names
			m_instantEvaluatorNames.reserve(numInstantEvaluatorBlueprints);
			for (const CInstantEvaluatorBlueprint* ieBP : instantEvaluatorBlueprints)
			{
				const char* name = ieBP->GetFactory().GetName();
				m_instantEvaluatorNames.push_back(name);
				m_longestEvaluatorName = std::max(m_longestEvaluatorName, strlen(name));
			}

			// deferred-evaluator names
			m_deferredEvaluatorNames.reserve(numDeferredEvaluatorBlueprints);
			for (const CDeferredEvaluatorBlueprint* deBP : deferredEvaluatorBlueprints)
			{
				const char* name = deBP->GetFactory().GetName();
				m_deferredEvaluatorNames.push_back(name);
				m_longestEvaluatorName = std::max(m_longestEvaluatorName, strlen(name));
			}
		}

		void CHistoricQuery::OnInstantEvaluatorScoredItem(size_t instantEvaluatorIndex, size_t itemIndex, float nonWeightedSingleScore, float accumulatedAndWeightedScoreSoFar)
		{
			SHistoricItem& item = m_items[itemIndex];
			SHistoricInstantEvaluatorResult& result = item.resultOfAllInstantEvaluators[instantEvaluatorIndex];
			assert(result.status == SHistoricInstantEvaluatorResult::EStatus::HasNotRunYet);
			result.nonWeightedScore = nonWeightedSingleScore;
			result.status = SHistoricInstantEvaluatorResult::EStatus::HasFinishedAndScoredTheItem;
			item.accumulatedAndWeightedScoreSoFar = accumulatedAndWeightedScoreSoFar;
		}

		void CHistoricQuery::OnInstantEvaluatorDiscardedItem(size_t instantEvaluatorIndex, size_t itemIndex)
		{
			SHistoricItem& item = m_items[itemIndex];
			SHistoricInstantEvaluatorResult& result = item.resultOfAllInstantEvaluators[instantEvaluatorIndex];
			assert(result.status == SHistoricInstantEvaluatorResult::EStatus::HasNotRunYet);
			result.status = SHistoricInstantEvaluatorResult::EStatus::HasFinishedAndDiscardedTheItem;
		}

		void CHistoricQuery::OnFunctionCallExceptionOccurredInInstantEvaluator(size_t instantEvaluatorIndex, size_t itemIndex, const char* exceptionMessage)
		{
			SHistoricItem& item = m_items[itemIndex];
			SHistoricInstantEvaluatorResult& result = item.resultOfAllInstantEvaluators[instantEvaluatorIndex];
			assert(result.status == SHistoricInstantEvaluatorResult::EStatus::HasNotRunYet);
			result.status = SHistoricInstantEvaluatorResult::EStatus::ExceptionOccurredInFunctionCall;
			result.furtherInformationAboutStatus = exceptionMessage;
		}

		void CHistoricQuery::OnExceptionOccurredInInstantEvaluator(size_t instantEvaluatorIndex, size_t itemIndex, const char* exceptionMessage)
		{
			SHistoricItem& item = m_items[itemIndex];
			SHistoricInstantEvaluatorResult& result = item.resultOfAllInstantEvaluators[instantEvaluatorIndex];
			assert(result.status == SHistoricInstantEvaluatorResult::EStatus::HasNotRunYet);
			result.status = SHistoricInstantEvaluatorResult::EStatus::ExceptionOccurredInHimself;
			result.furtherInformationAboutStatus = exceptionMessage;
		}

		void CHistoricQuery::OnDeferredEvaluatorStartedRunningOnItem(size_t deferredEvaluatorIndex, size_t itemIndex)
		{
			SHistoricItem& item = m_items[itemIndex];
			SHistoricDeferredEvaluatorResult& result = item.resultOfAllDeferredEvaluators[deferredEvaluatorIndex];
			assert(result.status == SHistoricDeferredEvaluatorResult::EStatus::HasNotRunYet);
			result.status = SHistoricDeferredEvaluatorResult::EStatus::IsRunningNow;
		}

		void CHistoricQuery::OnDeferredEvaluatorScoredItem(size_t deferredEvaluatorIndex, size_t itemIndex, float nonWeightedSingleScore, float accumulatedAndWeightedScoreSoFar)
		{
			SHistoricItem& item = m_items[itemIndex];
			SHistoricDeferredEvaluatorResult& result = item.resultOfAllDeferredEvaluators[deferredEvaluatorIndex];
			assert(result.status == SHistoricDeferredEvaluatorResult::EStatus::IsRunningNow);
			result.nonWeightedScore = nonWeightedSingleScore;
			result.status = SHistoricDeferredEvaluatorResult::EStatus::HasFinishedAndScoredTheItem;
			item.accumulatedAndWeightedScoreSoFar = accumulatedAndWeightedScoreSoFar;
		}

		void CHistoricQuery::OnDeferredEvaluatorDiscardedItem(size_t deferredEvaluatorIndex, size_t itemIndex)
		{
			SHistoricItem& item = m_items[itemIndex];
			SHistoricDeferredEvaluatorResult& result = item.resultOfAllDeferredEvaluators[deferredEvaluatorIndex];
			assert(result.status == SHistoricDeferredEvaluatorResult::EStatus::IsRunningNow);
			result.status = SHistoricDeferredEvaluatorResult::EStatus::HasFinishedAndDiscardedTheItem;
		}

		void CHistoricQuery::OnDeferredEvaluatorGotAborted(size_t deferredEvaluatorIndex, size_t itemIndex, const char* reasonForAbort)
		{
			SHistoricDeferredEvaluatorResult& result = m_items[itemIndex].resultOfAllDeferredEvaluators[deferredEvaluatorIndex];
			assert(result.status == SHistoricDeferredEvaluatorResult::EStatus::IsRunningNow);
			result.status = SHistoricDeferredEvaluatorResult::EStatus::GotAborted;
			result.furtherInformationAboutStatus = reasonForAbort;
		}

		void CHistoricQuery::OnFunctionCallExceptionOccurredInDeferredEvaluator(size_t deferredEvaluatorIndex, size_t itemIndex, const char* exceptionMessage)
		{
			SHistoricItem& item = m_items[itemIndex];
			SHistoricDeferredEvaluatorResult& result = item.resultOfAllDeferredEvaluators[deferredEvaluatorIndex];
			assert(result.status == SHistoricDeferredEvaluatorResult::EStatus::HasNotRunYet);
			result.status = SHistoricDeferredEvaluatorResult::EStatus::ExceptionOccurredInFunctionCall;
			result.furtherInformationAboutStatus = exceptionMessage;
		}

		void CHistoricQuery::OnExceptionOccurredInDeferredEvaluator(size_t deferredEvaluatorIndex, size_t itemIndex, const char* exceptionMessage)
		{
			SHistoricItem& item = m_items[itemIndex];
			SHistoricDeferredEvaluatorResult& result = item.resultOfAllDeferredEvaluators[deferredEvaluatorIndex];
			assert(result.status == SHistoricDeferredEvaluatorResult::EStatus::HasNotRunYet);
			result.status = SHistoricDeferredEvaluatorResult::EStatus::ExceptionOccurredInHimself;
			result.furtherInformationAboutStatus = exceptionMessage;
		}

		void CHistoricQuery::OnItemGotDisqualifiedDueToBadScore(size_t itemIndex)
		{
			m_items[itemIndex].bDisqualifiedDueToBadScore = true;
		}

		void CHistoricQuery::CreateItemDebugProxyViaItemFactoryForItem(const client::IItemFactory& itemFactory, const void* item, size_t indexInGeneratedItemsForWhichToCreateTheProxy)
		{
			itemFactory.CreateItemDebugProxyForItem(item, m_itemDebugProxyFactory);
			std::unique_ptr<CItemDebugProxyBase> freshlyCreatedItemProxy = m_itemDebugProxyFactory.GetAndForgetLastCreatedDebugItemRenderProxy();
			m_items[indexInGeneratedItemsForWhichToCreateTheProxy].pDebugProxy = std::move(freshlyCreatedItemProxy);
		}

		size_t CHistoricQuery::GetRoughMemoryUsage() const
		{
			size_t sizeOfSingleItem = 0;

			if (!m_items.empty())
			{
				// notice: the actual capacities are not considered, but shouldn't be higher than the actual element counts, since we only called resize() on the initial containers
				sizeOfSingleItem += sizeof(SHistoricItem);
				sizeOfSingleItem += m_items[0].resultOfAllInstantEvaluators.size() * sizeof(SHistoricInstantEvaluatorResult);
				sizeOfSingleItem += m_items[0].resultOfAllDeferredEvaluators.size() * sizeof(SHistoricDeferredEvaluatorResult);
			}

			const size_t sizeOfAllItems = sizeOfSingleItem * m_items.size();
			const size_t sizeOfDebugRenderWorld = m_debugRenderWorld.GetRoughMemoryUsage();
			return sizeOfAllItems + sizeOfDebugRenderWorld;
		}

		void CHistoricQuery::AnalyzeItemStatus(const SHistoricItem& itemToAnalyze, float bestScoreAmongAllItems, float worstScoreAmongAllItems, ColorF& outItemColor, bool& outShouldDrawItemScore, bool& outShouldDrawAnExclamationMarkAsWarning)
		{
			outItemColor = Col_Black;
			outShouldDrawItemScore = false;
			outShouldDrawAnExclamationMarkAsWarning = false;

			bool bHasEncounteredSomeException = false;

			bool allInstantEvaluatorsFinished = true;
			bool allDeferredEvaluatorsFinished = true;

			bool discardedByAtLeastOneInstantEvaluator = false;
			bool discardedByAtLeastOneDeferredEvaluator = false;

			for (const SHistoricInstantEvaluatorResult& ieResult : itemToAnalyze.resultOfAllInstantEvaluators)
			{
				switch (ieResult.status)
				{
				case SHistoricInstantEvaluatorResult::EStatus::HasFinishedAndScoredTheItem:
					// check for illegal score (one that is outside [0.0 .. 1.0] which can happen if an evaluator is implemented wrongly)
					if (ieResult.nonWeightedScore < 0.0f || ieResult.nonWeightedScore > 1.0f)
					{
						outShouldDrawAnExclamationMarkAsWarning = true;
					}
					break;

				case SHistoricInstantEvaluatorResult::EStatus::HasFinishedAndDiscardedTheItem:
					discardedByAtLeastOneInstantEvaluator = true;
					break;

				case SHistoricInstantEvaluatorResult::EStatus::HasNotRunYet:
					allInstantEvaluatorsFinished = false;
					break;

				case SHistoricInstantEvaluatorResult::EStatus::ExceptionOccurredInFunctionCall:
				case SHistoricInstantEvaluatorResult::EStatus::ExceptionOccurredInHimself:
					bHasEncounteredSomeException = true;
					outShouldDrawAnExclamationMarkAsWarning = true;
					break;

				default:
					// the remaining stati are not relevant for our purpose
					break;
				}
			}

			for (const SHistoricDeferredEvaluatorResult & deResult : itemToAnalyze.resultOfAllDeferredEvaluators)
			{
				switch (deResult.status)
				{
				case SHistoricDeferredEvaluatorResult::EStatus::HasFinishedAndScoredTheItem:
					// check for illegal score (one that is outside [0.0 .. 1.0] which can happen if an evaluator is implemented wrongly)
					if (deResult.nonWeightedScore < 0.0f || deResult.nonWeightedScore > 1.0f)
					{
						outShouldDrawAnExclamationMarkAsWarning = true;
					}
					break;

				case SHistoricDeferredEvaluatorResult::EStatus::HasFinishedAndDiscardedTheItem:
					discardedByAtLeastOneDeferredEvaluator = true;
					break;

				case SHistoricDeferredEvaluatorResult::EStatus::HasNotRunYet:
				case SHistoricDeferredEvaluatorResult::EStatus::GotAborted:
					allDeferredEvaluatorsFinished = false;
					break;

				case SHistoricDeferredEvaluatorResult::EStatus::ExceptionOccurredInFunctionCall:
				case SHistoricDeferredEvaluatorResult::EStatus::ExceptionOccurredInHimself:
					bHasEncounteredSomeException = true;
					outShouldDrawAnExclamationMarkAsWarning = true;
					break;

				default:
					// the remaining stati are not relevant for our purpose
					break;
				}
			}

			if (bHasEncounteredSomeException)
			{
				outItemColor = Col_Black;
			}
			else if (itemToAnalyze.bDisqualifiedDueToBadScore)
			{
				if (allInstantEvaluatorsFinished && allDeferredEvaluatorsFinished)
				{
					// all evaluators were running + finished
					outItemColor = Col_DarkGray;
					outShouldDrawItemScore = true;
				}
				else
				{
					// not all were running
					outItemColor = Col_Plum;
				}
			}
			else if (discardedByAtLeastOneInstantEvaluator || discardedByAtLeastOneDeferredEvaluator)
			{
				// item got discarded
				outItemColor = Col_Black;
			}
			else if (!allInstantEvaluatorsFinished || !allDeferredEvaluatorsFinished)
			{
				// item is still being evaluated
				outItemColor = Col_Yellow;
			}
			else
			{
				// it's one of the items in the result set => gradate its color from red to green, depending on its score

				const float range = bestScoreAmongAllItems - worstScoreAmongAllItems;
				const float itemRelativeScore = itemToAnalyze.accumulatedAndWeightedScoreSoFar - worstScoreAmongAllItems;
				const float fraction = (range > 0.0f) ? itemRelativeScore / range : 1.0f;

				outItemColor = Lerp(Col_Red, Col_Green, fraction);
				outShouldDrawItemScore = true;
			}
		}

		CTimeValue CHistoricQuery::ComputeElapsedTimeFromQueryCreationToDestruction() const
		{
			// elapsed time
			switch (m_queryLifetimeStatus)
			{
			case EQueryLifetimeStatus::QueryIsNotCreatedYet:
				return CTimeValue();

			case EQueryLifetimeStatus::QueryIsAlive:
				return (gEnv->pTimer->GetAsyncTime() - m_queryCreatedTimestamp);

			case EQueryLifetimeStatus::QueryIsDestroyed:
				return (m_queryDestroyedTimestamp - m_queryCreatedTimestamp);

			default:
				assert(0);
				return CTimeValue();
			}
		}

		bool CHistoricQuery::FindClosestItemInView(const SDebugCameraView& cameraView, size_t& outItemIndex) const
		{
			float closestDistanceSoFar = std::numeric_limits<float>::max();
			bool foundACloseEnoughItem = false;

			for (size_t i = 0, n = m_items.size(); i < n; ++i)
			{
				const SHistoricItem& item = m_items[i];

				if (item.pDebugProxy)
				{
					float dist;

					if (item.pDebugProxy->GetDistanceToCameraView(cameraView, dist) && dist < closestDistanceSoFar)
					{
						closestDistanceSoFar = dist;
						outItemIndex = i;
						foundACloseEnoughItem = true;
					}
				}
			}
			return foundACloseEnoughItem;
		}

		void CHistoricQuery::DrawDebugPrimitivesInWorld(size_t indexOfItemCurrentlyBeingFocused) const
		{
			m_debugRenderWorld.DrawAllAddedPrimitives(indexOfItemCurrentlyBeingFocused);

			//
			// - figure out the best and worst score of items that made it into the final result set
			// - we need these to get a "relative fitness" for each item do draw it with gradating color
			//

			float bestScoreAmongAllItems = std::numeric_limits<float>::min();
			float worstScoreAmongAllItems = std::numeric_limits<float>::max();

			for (const SHistoricItem& itemToAnalyze : m_items)
			{
				//
				// check if the item was prematurely disqualified due to a too bad score
				//

				if (itemToAnalyze.bDisqualifiedDueToBadScore)
					continue;

				//
				// check if the item survived all instant-evaluators
				//

				{
					bool bItemHasSurvivedAllInstantEvaluators = true;

					for (const SHistoricInstantEvaluatorResult& ieResult : itemToAnalyze.resultOfAllInstantEvaluators)
					{
						if (ieResult.status != SHistoricInstantEvaluatorResult::EStatus::HasFinishedAndScoredTheItem)
						{
							bItemHasSurvivedAllInstantEvaluators = false;
							break;
						}
					}

					if (!bItemHasSurvivedAllInstantEvaluators)
						continue;
				}

				//
				// check if the item survived all deferred-evaluators
				//

				{
					bool bItemHasSurvivedAllDeferredEvaluators = true;

					for (const SHistoricDeferredEvaluatorResult& deResult : itemToAnalyze.resultOfAllDeferredEvaluators)
					{
						if (deResult.status != SHistoricDeferredEvaluatorResult::EStatus::HasFinishedAndScoredTheItem)
						{
							bItemHasSurvivedAllDeferredEvaluators = false;
							break;
						}
					}

					if (!bItemHasSurvivedAllDeferredEvaluators)
						continue;
				}

				//
				// the item survived all evaluators => update the range of best/worst score
				//

				bestScoreAmongAllItems = std::max(bestScoreAmongAllItems, itemToAnalyze.accumulatedAndWeightedScoreSoFar);
				worstScoreAmongAllItems = std::min(worstScoreAmongAllItems, itemToAnalyze.accumulatedAndWeightedScoreSoFar);
			}

			//
			// draw all items in the debug-renderworld
			//

			for (size_t i = 0, n = m_items.size(); i < n; ++i)
			{
				const SHistoricItem& item = m_items[i];
				const bool bShowDetails = (indexOfItemCurrentlyBeingFocused == i);

				if (item.pDebugProxy)
				{
					ColorF color = Col_Black;
					bool bDrawScore = false;
					bool bShouldDrawAnExclamationMarkAsWarning = false;

					AnalyzeItemStatus(item, bestScoreAmongAllItems, worstScoreAmongAllItems, color, bDrawScore, bShouldDrawAnExclamationMarkAsWarning);

					item.pDebugProxy->Draw(color, bShowDetails);

					if (bDrawScore)
					{
						m_debugRenderWorld.DrawText(item.pDebugProxy->GetPivot(), 1.5f, color, "%f", item.accumulatedAndWeightedScoreSoFar);
					}

					if (bShowDetails)
					{
						// # item index
						m_debugRenderWorld.DrawText(item.pDebugProxy->GetPivot() + Vec3(0, 0, 1), 1.5f, color, "#%i", (int)i);
					}

					if (bShouldDrawAnExclamationMarkAsWarning)
					{
						// big exclamation mark to grab the user's attention that something is wrong and that he should inspect that particular item in detail

						float textSize = 5.0f;

						if (bShowDetails)
						{
							// pulsate the text
							textSize *= CDebugRenderPrimitiveBase::Pulsate();
						}

						m_debugRenderWorld.DrawText(item.pDebugProxy->GetPivot() + Vec3(0, 0, 1), textSize, Col_Red, "!");
					}
				}
			}
		}

		void CHistoricQuery::FillQueryHistoryConsumerWithShortInfoAboutQuery(IQueryHistoryConsumer& consumer, bool bHighlight) const
		{
			ColorF color;
			if (m_bExceptionOccurred)  // TODO: we should also draw in red when some items received invalid scores (outside [0.0 .. 1.0])
			{
				color = bHighlight ? Col_Red : Col_DeepPink;
			}
			else
			{
				color = bHighlight ? Col_Cyan : Col_White;
			}

			const size_t numGeneratedItems = m_finalStatistics.numGeneratedItems;

			// tentative: - the actual items may have come from a child query whereas the parent query didn't even generate items
			//            - so we rely on CQuery_SequentialBase::OnGetStatistics() to have counted the final items from the result set of one of its child queries
			const size_t numItemsInFinalResultSet = m_finalStatistics.numItemsInFinalResultSet;

			const CTimeValue elapsedTime = ComputeElapsedTimeFromQueryCreationToDestruction();
			const IQueryHistoryConsumer::SHistoricQueryOverview overview(
				color,
				m_querierName.c_str(),
				m_queryID,
				m_parentQueryID,
				m_queryBlueprintName.c_str(),
				numGeneratedItems,
				numItemsInFinalResultSet,
				elapsedTime);
			consumer.AddHistoricQuery(overview);
		}

		void CHistoricQuery::FillQueryHistoryConsumerWithDetailedInfoAboutQuery(IQueryHistoryConsumer& consumer) const
		{
			const ColorF color = Col_White;

			// query ID
			{
				shared::CUqsString queryIdAsString;
				m_queryID.ToString(queryIdAsString);
				consumer.AddTextLineToCurrentHistoricQuery(color, "=== Query #%s ===", queryIdAsString.c_str());
			}

			// querier name + query blueprint name
			{
				consumer.AddTextLineToCurrentHistoricQuery(color, "'%s' / '%s'", m_finalStatistics.querierName.c_str(), m_finalStatistics.queryBlueprintName.c_str());
			}

			// exception message
			{
				if (m_bExceptionOccurred)
				{
					consumer.AddTextLineToCurrentHistoricQuery(Col_Red, "Exception: %s", m_exceptionMessage.c_str());
				}
				else
				{
					consumer.AddTextLineToCurrentHistoricQuery(color, "No exception");
				}
			}

			// elapsed frames and time
			{
				// elapsed frames
				consumer.AddTextLineToCurrentHistoricQuery(color, "elapsed frames until result:  %i", (int)m_finalStatistics.totalElapsedFrames);

				// elapsed time (this is NOT the same as the *consumed* time)
				const float elapsedTimeInMS = ComputeElapsedTimeFromQueryCreationToDestruction().GetMilliSeconds();
				consumer.AddTextLineToCurrentHistoricQuery(color, "elapsed seconds until result: %f (%.2f milliseconds)", elapsedTimeInMS / 1024.0f, elapsedTimeInMS);

				// consumed time (this is the accumulation of the granted and consumed amounts of time per update call while the query was running)
				consumer.AddTextLineToCurrentHistoricQuery(color, "consumed seconds:             %f (%.2f milliseconds)", m_finalStatistics.totalConsumedTime.GetSeconds(), m_finalStatistics.totalConsumedTime.GetMilliSeconds());
			}

			// canceled: yes/no
			{
				consumer.AddTextLineToCurrentHistoricQuery(color, "canceled prematurely:         %s", m_bGotCanceledPrematurely ? "YES" : "NO");
			}

			// number of generated items, remaining items to inspect, final items
			{
				// generated items
				consumer.AddTextLineToCurrentHistoricQuery(color, "items generated:              %i", (int)m_finalStatistics.numGeneratedItems);

				// remaining items to inspect
				consumer.AddTextLineToCurrentHistoricQuery(color, "items still to inspect:       %i", (int)m_finalStatistics.numRemainingItemsToInspect);

				// final items
				consumer.AddTextLineToCurrentHistoricQuery(color, "final items:                  %i", (int)m_finalStatistics.numItemsInFinalResultSet);
			}

			// memory usage
			{
				// items
				consumer.AddTextLineToCurrentHistoricQuery(color, "items memory:                 %i (%i kb)", (int)m_finalStatistics.memoryUsedByGeneratedItems, (int)m_finalStatistics.memoryUsedByGeneratedItems / 1024);

				// working data of items
				consumer.AddTextLineToCurrentHistoricQuery(color, "items working data memory:    %i (%i kb)", (int)m_finalStatistics.memoryUsedByItemsWorkingData, (int)m_finalStatistics.memoryUsedByItemsWorkingData / 1024);
			}

			// Instant-Evaluators
			{
				const size_t numInstantEvaluators = m_finalStatistics.instantEvaluatorsRuns.size();
				for (size_t i = 0; i < numInstantEvaluators; ++i)
				{
					consumer.AddTextLineToCurrentHistoricQuery(color, "Instant-Evaluator #%i:  full runs = %i", (int)i, (int)m_finalStatistics.instantEvaluatorsRuns[i]);
				}
			}

			// Deferred-Evaluators
			{
				const size_t numDeferredEvaluators = m_finalStatistics.deferredEvaluatorsFullRuns.size();
				for (size_t i = 0; i < numDeferredEvaluators; ++i)
				{
					consumer.AddTextLineToCurrentHistoricQuery(color, "Deferred-Evaluator #%i: full runs = %i, aborted runs = %i", (int)i, (int)m_finalStatistics.deferredEvaluatorsFullRuns[i], (int)m_finalStatistics.deferredEvaluatorsAbortedRuns[i]);
				}
			}

			// Phases
			{
				const size_t numPhases = m_finalStatistics.elapsedTimePerPhase.size();
				for (size_t i = 0; i < numPhases; ++i)
				{
					// print a human-readable phase index
					consumer.AddTextLineToCurrentHistoricQuery(color, "Phase '%i'  = %i frames, %f sec (%.2f ms) [longest call = %f sec (%.2f ms)]",
						(int)i + 1,
						(int)m_finalStatistics.elapsedFramesPerPhase[i],
						m_finalStatistics.elapsedTimePerPhase[i].GetSeconds(),
						m_finalStatistics.elapsedTimePerPhase[i].GetSeconds() * 1000.0f,
						m_finalStatistics.peakElapsedTimePerPhaseUpdate[i].GetSeconds(),
						m_finalStatistics.peakElapsedTimePerPhaseUpdate[i].GetSeconds() * 1000.0f);
				}
			}
		}

		void CHistoricQuery::FillQueryHistoryConsumerWithDetailedInfoAboutItem(IQueryHistoryConsumer& consumer, size_t itemIndex) const
		{
			assert(itemIndex < m_items.size());

			const SHistoricItem& item = m_items[itemIndex];

			// track which instant- and deferred-evaluators were implemented wrongly and gave the item a score outside the valid range [0.0 .. 1.0]
			std::vector<size_t> indexesOfInstantEvaluatorsWithIllegalScores;
			std::vector<size_t> indexesOfDeferredEvaluatorsWithIllegalScores;

			// item #
			consumer.AddTextLineToFocusedItem(Col_White, "--- item #%i ---", (int)itemIndex);

			// empty line
			consumer.AddTextLineToFocusedItem(Col_White, "");

			// item score
			consumer.AddTextLineToFocusedItem(Col_White, "final score: %f", item.accumulatedAndWeightedScoreSoFar);
			consumer.AddTextLineToFocusedItem(Col_White, "disqualified due to bad score: %s", item.bDisqualifiedDueToBadScore ? "yes" : "no");

			// empty line
			consumer.AddTextLineToFocusedItem(Col_White, "");

			// instant evaluators
			for (size_t i = 0, n = item.resultOfAllInstantEvaluators.size(); i < n; ++i)
			{
				const SHistoricInstantEvaluatorResult& ieResult = item.resultOfAllInstantEvaluators[i];
				ColorF ieColor = Col_White;
				stack_string text = "?";

				switch (ieResult.status)
				{
				case SHistoricInstantEvaluatorResult::EStatus::HasNotRunYet:
					text = "has not run yet";
					ieColor = Col_Plum;
					break;

				case SHistoricInstantEvaluatorResult::EStatus::HasFinishedAndDiscardedTheItem:
					text = "discarded the item";
					ieColor = Col_Red;
					break;

				case SHistoricInstantEvaluatorResult::EStatus::HasFinishedAndScoredTheItem:
					text.Format("scored the item: %f", ieResult.nonWeightedScore);
					ieColor = Col_Green;
					if (ieResult.nonWeightedScore < 0.0f || ieResult.nonWeightedScore > 1.0f)
					{
						indexesOfInstantEvaluatorsWithIllegalScores.push_back(i);
					}
					break;

				case SHistoricInstantEvaluatorResult::EStatus::ExceptionOccurredInFunctionCall:
					text.Format("exception in function call: %s", ieResult.furtherInformationAboutStatus.c_str());
					ieColor = Col_Red;
					break;

				case SHistoricInstantEvaluatorResult::EStatus::ExceptionOccurredInHimself:
					text.Format("exception in himself: %s", ieResult.furtherInformationAboutStatus.c_str());
					ieColor = Col_Red;
					break;
				}

				consumer.AddTextLineToFocusedItem(ieColor, "IE #%i [%-*s]: %s", (int)i, (int)m_longestEvaluatorName, m_instantEvaluatorNames[i].c_str(), text.c_str());
			}

			// empty line
			consumer.AddTextLineToFocusedItem(Col_White, "");

			// deferred evaluators
			for (size_t i = 0, n = item.resultOfAllDeferredEvaluators.size(); i < n; ++i)
			{
				const SHistoricDeferredEvaluatorResult& deResult = item.resultOfAllDeferredEvaluators[i];
				ColorF deColor = Col_White;
				stack_string text = "?";

				switch (deResult.status)
				{
				case SHistoricDeferredEvaluatorResult::EStatus::HasNotRunYet:
					text = "has not run yet";
					deColor = Col_Plum;
					break;

				case SHistoricDeferredEvaluatorResult::EStatus::IsRunningNow:
					text = "running now";
					deColor = Col_Yellow;
					break;

				case SHistoricDeferredEvaluatorResult::EStatus::HasFinishedAndDiscardedTheItem:
					text = "discarded the item";
					deColor = Col_Red;
					break;

				case SHistoricDeferredEvaluatorResult::EStatus::HasFinishedAndScoredTheItem:
					text.Format("scored the item: %f", deResult.nonWeightedScore);
					deColor = Col_Green;
					if (deResult.nonWeightedScore < 0.0f || deResult.nonWeightedScore > 1.0f)
					{
						indexesOfDeferredEvaluatorsWithIllegalScores.push_back(i);
					}
					break;

				case SHistoricDeferredEvaluatorResult::EStatus::GotAborted:
					text.Format("got aborted prematurely: %s", deResult.furtherInformationAboutStatus.c_str());
					deColor = Col_DarkGray;
					break;

				case SHistoricDeferredEvaluatorResult::EStatus::ExceptionOccurredInFunctionCall:
					text.Format("exception in function call: %s", deResult.furtherInformationAboutStatus.c_str());
					deColor = Col_Red;
					break;

				case SHistoricDeferredEvaluatorResult::EStatus::ExceptionOccurredInHimself:
					text.Format("exception in himself: %s", deResult.furtherInformationAboutStatus.c_str());
					deColor = Col_Red;
					break;
				}

				consumer.AddTextLineToFocusedItem(deColor, "DE #%i [%-*s]: %s", (int)i, (int)m_longestEvaluatorName, m_deferredEvaluatorNames[i].c_str(), text.c_str());
			}

			// print the evaluators that gave scores outside the valid range [0.0 .. 1.0]
			if (!indexesOfInstantEvaluatorsWithIllegalScores.empty() || !indexesOfDeferredEvaluatorsWithIllegalScores.empty())
			{
				// empty line
				consumer.AddTextLineToFocusedItem(Col_White, "");

				consumer.AddTextLineToFocusedItem(Col_Red, "WARNING: these evaluators gave an illegal score (scores must always be in [0.0 .. 1.0]):");

				// empty line
				consumer.AddTextLineToFocusedItem(Col_White, "");

				for (size_t i = 0, n = indexesOfInstantEvaluatorsWithIllegalScores.size(); i < n; ++i)
				{
					const size_t index = indexesOfInstantEvaluatorsWithIllegalScores[i];
					const char* ieName = m_instantEvaluatorNames[index];
					const SHistoricInstantEvaluatorResult& ieResult = item.resultOfAllInstantEvaluators[index];
					consumer.AddTextLineToFocusedItem(Col_Red, "IE #%i [%-*s]: %f", (int)index, (int)m_longestEvaluatorName, ieName, ieResult.nonWeightedScore);
				}

				for (size_t i = 0, n = indexesOfDeferredEvaluatorsWithIllegalScores.size(); i < n; ++i)
				{
					const size_t index = indexesOfDeferredEvaluatorsWithIllegalScores[i];
					const char* deName = m_deferredEvaluatorNames[index];
					const SHistoricDeferredEvaluatorResult& deResult = item.resultOfAllDeferredEvaluators[index];
					consumer.AddTextLineToFocusedItem(Col_Red, "DE #%i [%-*s]: %f", (int)index, (int)m_longestEvaluatorName, deName, deResult.nonWeightedScore);
				}
			}
		}

		const CQueryID& CHistoricQuery::GetQueryID() const
		{
			return m_queryID;
		}

		const CQueryID& CHistoricQuery::GetParentQueryID() const
		{
			return m_parentQueryID;
		}

		bool CHistoricQuery::IsQueryDestroyed() const
		{
			return m_queryLifetimeStatus == EQueryLifetimeStatus::QueryIsDestroyed;
		}

		const CTimeValue& CHistoricQuery::GetQueryDestroyedTimestamp() const
		{
			return m_queryDestroyedTimestamp;
		}

		void CHistoricQuery::Serialize(Serialization::IArchive& ar)
		{
			ar(m_queryID, "m_queryID");
			ar(m_parentQueryID, "m_parentQueryID");
			ar(m_querierName, "m_querierName");
			ar(m_queryBlueprintName, "m_queryBlueprintName");
			ar(m_queryLifetimeStatus, "m_queryLifetimeStatus");
			ar(m_queryCreatedTimestamp, "m_queryCreatedTimestamp");
			ar(m_queryDestroyedTimestamp, "m_queryDestroyedTimestamp");
			ar(m_bGotCanceledPrematurely, "m_bGotCanceledPrematurely");
			ar(m_bExceptionOccurred, "m_bExceptionOccurred");
			ar(m_exceptionMessage, "m_exceptionMessage");
			ar(m_debugRenderWorld, "m_debugRenderWorld");
			ar(m_items, "m_items");
			ar(m_instantEvaluatorNames, "m_instantEvaluatorNames");
			ar(m_deferredEvaluatorNames, "m_deferredEvaluatorNames");
			ar(m_longestEvaluatorName, "m_longestEvaluatorName");
			ar(m_finalStatistics, "m_finalStatistics");
			ar(m_itemDebugProxyFactory, "m_itemDebugProxyFactory");		// notice: it wouldn't even be necessary to serialize the CItemDebugProxyFactory as its only member variables is just used for intermediate storage and will always be set to its default value at the time of serialization
		}

		//===================================================================================
		//
		// CQueryHistory
		//
		//===================================================================================

		CQueryHistory::CQueryHistory()
		{
			// nothing
		}

		CQueryHistory& CQueryHistory::operator=(CQueryHistory&& other)
		{
			if (this != &other)
			{
				m_history = std::move(other.m_history);
				m_metaData = std::move(other.m_metaData);
			}
			return *this;
		}

		HistoricQuerySharedPtr CQueryHistory::AddNewHistoryEntry(const CQueryID& queryID, const char* querierName, const CQueryID& parentQueryID, CQueryHistoryManager* pOwningHistoryManager)
		{
			//
			// insert the new historic query into the correct position in the array such that children will always reside somewhere after their parent
			// (this gives us the desired depth-first traversal when scrolling through all historic queries)
			//

			auto insertPos = m_history.end();

			if (parentQueryID.IsValid())
			{
				for (auto it = m_history.begin(); it != m_history.end(); )
				{
					const CHistoricQuery* pCurrentHistoricQuery = it->get();

					++it;

					// found our parent? -> remember for possibly inserting after him
					if (pCurrentHistoricQuery->GetQueryID() == parentQueryID)
					{
						insertPos = it;
					}
					// found another child of our parent? -> remember for possibly inserting after it
					else if (pCurrentHistoricQuery->GetParentQueryID() == parentQueryID)
					{
						insertPos = it;
					}
				}
			}

			HistoricQuerySharedPtr pNewEntry(new CHistoricQuery(queryID, querierName, parentQueryID, pOwningHistoryManager));
			insertPos = m_history.insert(insertPos, std::move(pNewEntry));
			return *insertPos;
		}

		void CQueryHistory::Clear()
		{
			m_history.clear();
			// keep m_metaData
		}

		size_t CQueryHistory::GetHistorySize() const
		{
			return m_history.size();
		}

		const CHistoricQuery& CQueryHistory::GetHistoryEntryByIndex(size_t index) const
		{
			assert(index < m_history.size());
			return *m_history[index];
		}

		const CHistoricQuery* CQueryHistory::FindHistoryEntryByQueryID(const CQueryID& queryID) const
		{
			for (const HistoricQuerySharedPtr& pHistoricEntry : m_history)
			{
				if (pHistoricEntry->GetQueryID() == queryID)
				{
					return pHistoricEntry.get();
				}
			}
			return nullptr;
		}

		size_t CQueryHistory::GetRoughMemoryUsage() const
		{
			size_t memoryUsed = 0;

			for (const HistoricQuerySharedPtr& q : m_history)
			{
				memoryUsed += q->GetRoughMemoryUsage();
			}

			return memoryUsed;
		}

		void CQueryHistory::SetArbitraryMetaDataForSerialization(const char* key, const char* value)
		{
			m_metaData[key] = value;
		}

		void CQueryHistory::Serialize(Serialization::IArchive& ar)
		{
			ar(m_metaData, "m_metaData");
			ar(m_history, "m_history");
		}

	}
}
