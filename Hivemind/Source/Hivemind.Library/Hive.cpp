#include "pch.h"
#include "Hive.h"


using namespace std;

Hive::Hive(const sf::Vector2f& position) :
	Entity(position, sf::Color(196, 196, 196), sf::Color(222, 147, 12)), mDimensions(STANDARD_WIDTH, STANDARD_HEIGHT), mBody(mDimensions),
	mFoodAmount(5000.0f), mText(), mGenerator(), mWaggleDanceClock(), mWaggleDanceWaitPeriod(Bee::STANDARD_HARVESTING_DURATION), mWaggleDanceInProgress(false),
	mStructuralComb(2000.0f), mHoneyComb(5000.0f), mBroodComb(550.0f),
	mOnlookerCount(0), mEmployeeCount(0), mGuardCount(0), mQueenCount(0), mDroneCount(0),
	mHUD(mPosition + sf::Vector2f(-(mDimensions.x / 2.0f), mDimensions.y + 30), sf::Vector2f(mDimensions.x * 2, 20),
		mOnlookerCount, mEmployeeCount, mDroneCount, mGuardCount, mQueenCount, mStructuralComb, mHoneyComb, mBroodComb, mFoodAmount)
{
	std::random_device device;
	mGenerator = std::default_random_engine(device());
	mFoodSourceData.clear();
	mBody.setPosition(mPosition);
	mBody.setOutlineThickness(14);
	mBody.setOutlineColor(mOutlineColor);
	mBody.setFillColor(mFillColor);

	mText.setPosition(mPosition.x - mText.getLocalBounds().width / 2, mPosition.y);
	mText.setFont(FontManager::GetInstance()->Hack());
	mText.setCharacterSize(16);
	mText.setOutlineColor(sf::Color::White);
}

Hive::~Hive()
{
	mIdleBees.clear();
	mFoodSourceData.clear();
}

void Hive::Update(sf::RenderWindow& window, const double& deltaTime)
{
	UNREFERENCED_PARAMETER(window);
	UNREFERENCED_PARAMETER(deltaTime);

	if (mCollisionNode != nullptr && !mCollisionNode->ContainsPoint(mPosition))
	{	// If we haev a collision node and we leave it, invalidate the pointer
		mCollisionNode->UnregisterHive(this);
		mCollisionNode = nullptr;
	}

	if (mCollisionNode == nullptr)
	{	// If the collision node is invalidated, get a new one and register to it
		mCollisionNode = CollisionGrid::GetInstance()->CollisionNodeFromPosition(mPosition);
		mCollisionNode->RegisterHive(this);
	}

//	std::stringstream ss;
//	ss << "Food: " << mFoodAmount << endl << endl;
//	mText.setString(ss.str());
//	mText.setPosition(mPosition.x + 30, mPosition.y);

	mHUD.UpdateHUDValues();
}

void Hive::Render(sf::RenderWindow& window) const
{
	window.draw(mBody);
	window.draw(mText);
	mHUD.Render(window);
}

sf::Vector2f Hive::GetCenterTarget() const
{
	return sf::Vector2f(mPosition.x + mDimensions.x / 2, mPosition.y + mDimensions.y / 2);
}

const sf::Vector2f& Hive::GetDimensions() const
{
	return mDimensions;
}

float Hive::GetFoodAmount() const
{
	return mFoodAmount;
}

void Hive::DepositFood(const float& foodAmount)
{
	mFoodAmount += foodAmount;
}

float Hive::TakeFood(float foodAmount)
{
	if (mFoodAmount < foodAmount)
	{
		foodAmount = mFoodAmount;
	}
	mFoodAmount -= foodAmount;
	return foodAmount;
}

void Hive::AddIdleBee(OnlookerBee* const bee)
{
	bool containsBee = false;

	for (auto iter = mIdleBees.begin(); iter != mIdleBees.end(); ++iter)
	{
		if (*iter == bee)
		{
			containsBee = true;
			break;
		}
	}

	if (!containsBee)
	{	// Only add the bee if it does not exist in the collection already
		mIdleBees.push_back(bee);
	}
}

void Hive::RemoveIdleBee(OnlookerBee* const bee)
{
	for (auto iter = mIdleBees.begin(); iter != mIdleBees.end(); ++iter)
	{
		if (*iter == bee)
		{	// If we encounter the bee, remove it. If not, no behavior
			mIdleBees.erase(iter);
			break;
		}
	}
}

std::vector<OnlookerBee*>::iterator Hive::IdleBeesBegin()
{
	return mIdleBees.begin();
}

std::vector<OnlookerBee*>::iterator Hive::IdleBeesEnd()
{
	return mIdleBees.end();
}

void Hive::ValidateIdleBees()
{
	bool beeRemoved = true;
	while (beeRemoved)
	{	// Repeat until we look through all bees and all are idle
		beeRemoved = false;
		for (auto iter = mIdleBees.begin(); iter != mIdleBees.end(); ++iter)
		{
			if ((*iter)->GetState() != Bee::State::Idle)
			{	// We know it isn't idle, so remove it
				beeRemoved = true;
				mIdleBees.erase(iter);
				break;
			}
		}
	}
}

void Hive::UpdateKnownFoodSource(FoodSource* const foodSource, const std::pair<float, float>& foodSourceData)
{
	mFoodSourceData[foodSource] = foodSourceData;
}

void Hive::RemoveFoodSource(FoodSource* const foodSource)
{
	for (auto iter = mFoodSourceData.begin(); iter != mFoodSourceData.end(); ++iter)
	{
		if (iter->first == foodSource)
		{
			mFoodSourceData.erase(iter);
			break;
		}
	}
}

void Hive::TriggerWaggleDance()
{
	if (mFoodSourceData.size() > 8)
	{
		mWaggleDanceClock.restart();
		mWaggleDanceInProgress = true;
		CompleteWaggleDance();
		mFoodSourceData.clear();
	}
}

void Hive::CompleteWaggleDance()
{
	if (mFoodSourceData.size() == 0)
	{
		return;
	}

	std::vector<std::pair<FoodSource*, float>> fitnessWeights;
	float fitnessSum = 0.0f;

	float minYield = mFoodSourceData.begin()->second.first;
	float maxYield = mFoodSourceData.begin()->second.first;
	float minDistance = mFoodSourceData.begin()->second.second;
	float maxDistance = mFoodSourceData.begin()->second.second;
	for (auto iter = mFoodSourceData.begin(); iter != mFoodSourceData.end(); ++iter)
	{	// Determine the range of values to help determine fitness
		auto pair = iter->second;
		if (pair.first < minYield)
		{
			minYield = pair.first;
		}
		if (pair.first > maxYield)
		{
			maxYield = pair.first;
		}

		if (pair.second < minDistance)
		{
			minDistance = pair.second;
		}
		if (pair.second > maxDistance)
		{
			maxDistance = pair.second;
		}
	}

	for (auto iter = mFoodSourceData.begin(); iter != mFoodSourceData.end(); ++iter)
	{
		float weight = ComputeFitness(iter->second, minYield, maxYield, minDistance, maxDistance);
		std::pair<FoodSource* const, float> pair(iter->first, weight);
		fitnessWeights.push_back(pair);
		fitnessSum += weight;
	}
	
	std::sort(fitnessWeights.begin(), fitnessWeights.end(), 
		[](const std::pair<FoodSource* const, float>& lhs, const std::pair<FoodSource* const, float>& rhs)
	{
		return lhs.second > rhs.second;
	});

	for (auto iter = IdleBeesBegin(); iter != IdleBeesEnd(); ++iter)
	{
		assert(*iter != nullptr);

		std::uniform_real_distribution<float> distribution(0, fitnessSum);
		float roll = distribution(mGenerator);
		for (auto fitnessIter = fitnessWeights.begin(); fitnessIter != fitnessWeights.end(); ++fitnessIter)
		{
			roll -= fitnessIter->second;
			if (roll < fitnessIter->second)
			{
				if (*iter != nullptr && fitnessIter->first != nullptr)
				{
					(*iter)->SetTarget(fitnessIter->first);
					(*iter)->SetState(Bee::State::SeekingTarget);
				}
				break;
			}
		}
	}

	ValidateIdleBees();
	mWaggleDanceInProgress = false;
}

void Hive::AddStructuralComb(const float& combAmount)
{
	mStructuralComb += combAmount;
}

void Hive::RemoveStructuralComb(const float& combAmount)
{
	if (mStructuralComb > combAmount)
	{
		mStructuralComb -= combAmount;
	}
	else
	{
		mStructuralComb = 0;
	}
}

void Hive::ConvertToHoneyComb(float combAmount)
{
	if (combAmount > mStructuralComb)
	{
		combAmount = mStructuralComb;
	}
	mStructuralComb -= combAmount;
	mHoneyComb += combAmount;
}

void Hive::ConvertToBroodComb(float combAmount)
{
	if (combAmount > mStructuralComb)
	{
		combAmount = mStructuralComb;
	}
	mStructuralComb -= combAmount;
	mBroodComb += combAmount;
}

float Hive::ComputeFitness(const std::pair<float, float>& foodData,
	const float& minYield, const float& maxYield,
	const float& minDistance, const float& maxDistance)
{
	float offsetFromMinYield = foodData.first - minYield;
	float yieldRange = maxYield - minYield;

	float offsetFromMaxDistance = maxDistance - foodData.second;
	float distanceRange = maxDistance - minDistance;

	float result;
	if (yieldRange == 0.0f && distanceRange == 0.0f)
	{	// Avoid dividing by zero and apply uniform fitness
		result = 1.0f;
	}
	else if (yieldRange == 0.0f)
	{	// We apply uniform yield weight and compute distance
		result = ((1.0f) + (offsetFromMaxDistance / distanceRange)) / 2.0f;
	}
	else if (distanceRange == 0.0f)
	{	// We apply uniform distance weight and compute yield
		result = ((offsetFromMinYield / yieldRange) + (1.0f) / 2.0f);
	}
	else
	{	// We have two valid values
		result = ((offsetFromMinYield / yieldRange) + (offsetFromMaxDistance / distanceRange)) / 2.0f;
	}
	return result;
}

bool Hive::RequiresStructuralComb() const
{
	return mStructuralComb < 2000.0f;
}

bool Hive::RequiresHoneyComb() const
{
	return static_cast<uint32_t>(mHoneyComb) < mFoodAmount;
}

bool Hive::RequiresBroodComb() const
{
	auto beeSum = mOnlookerCount + mEmployeeCount + mQueenCount + mDroneCount + mGuardCount;
	return static_cast<int>(mBroodComb) < beeSum;
}

void Hive::IncrementBeeCount(const BeeType& type)
{
	switch (type)
	{
	case Drone: 
		mDroneCount++;
		break;
	case Employee: 
		mEmployeeCount++;
		break;
	case Onlooker: 
		mOnlookerCount++;
		break;
	case Queen: 
		mQueenCount++;
		break;
	case Guard: 
		mGuardCount++;
		break;
	}
}

void Hive::DecrementBeeCount(const BeeType& type)
{
	switch (type)
	{
	case Drone:
		mDroneCount--;
		break;
	case Employee:
		mEmployeeCount--;
		break;
	case Onlooker:
		mOnlookerCount--;
		break;
	case Queen:
		mQueenCount--;
		break;
	case Guard:
		mGuardCount--;
		break;
	}
}

int Hive::GetBeeCount(const Bee::Type& type) const
{
	int result = 0;

	switch (type)
	{
	case Bee::Onlooker: 
		result = mOnlookerCount;
		break;
	case Bee::Employee: 
		result = mEmployeeCount;
		break;
	case Bee::Drone: 
		result = mDroneCount;
		break;
	case Bee::Guard: 
		result = mGuardCount;
		break;
	case Bee::Queen: 
		result = mQueenCount;
		break;
	}

	return result;
}

bool Hive::FoodSourceIsKnown(FoodSource* const foodSource) const
{
	bool result = false;
	for (auto iter = mFoodSourceData.begin(); iter != mFoodSourceData.end(); ++iter)
	{
		if ((*iter).first == foodSource)
		{
			result = true;
			break;
		}
	}
	return result;
}
