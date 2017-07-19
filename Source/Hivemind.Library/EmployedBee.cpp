#include "pch.h"
#include "EmployedBee.h"
#include "Hive.h"
#include "FoodSource.h"
#include "FoodSourceManager.h"


using namespace std;

EmployedBee::EmployedBee(const sf::Vector2f& position, Hive& hive) :
	Bee(position, hive), mPairedFoodSource(nullptr), mFlowField(FlowFieldManager::GetInstance()->GetField()), mDisplayFlowField(false),
	mLineToFoodSource(sf::LineStrip, 2), mFoodSourceData(0.0f, 0.0f), mAbandoningFoodSource(false)
{
	mState = State::Scouting;

	mFillColor = sf::Color::Cyan;
	mBody.setFillColor(mFillColor);
}

void EmployedBee::Update(sf::RenderWindow& window, const float& deltaTime)
{
	Bee::Update(window, deltaTime);

	UpdateFlowField(window, deltaTime);

	switch (mState)
	{
	case Scouting:
		UpdateScouting(window, deltaTime);
		break;
	case State::SeekingTarget:
		UpdateSeekingTarget(window, deltaTime);
		break;
	case State::HarvestingFood:
		UpdateHarvestingFood(window, deltaTime);
		break;
	case State::DeliveringFood:
		UpdateDeliveringFood(window, deltaTime);
		break;
	case State::DepositingFood:
		UpdateDepositingFood(window, deltaTime);
		break;
	default:
		break;
	}

	if (mPairedFoodSource != nullptr)
	{
		mLineToFoodSource[0].position = mPosition;
		mLineToFoodSource[0].color = sf::Color(255, 0, 0, 64);
		mLineToFoodSource[1].position = mPairedFoodSource->GetCenterTarget();
		mLineToFoodSource[1].color = sf::Color(255, 0, 0, 64);
	}

	stringstream ss;
	ss << static_cast<int>(mVelocity.x) << ", " << static_cast<int>(mVelocity.y);
	mText.setString(ss.str());
	mText.setPosition(mPosition - sf::Vector2f(mText.getLocalBounds().width / 2.0f, 35));
}

void EmployedBee::Render(sf::RenderWindow& window) const
{
	Bee::Render(window);
	if (mState == State::Scouting && mDisplayFlowField)
	{
		mFlowField.Render(window);
	}

	if (mPairedFoodSource != nullptr && mState != State::Scouting)
	{
		window.draw(mLineToFoodSource);
	}
//	if (mState == State::Scouting)
//	{
//		window.draw(mText);
//	}
}

void EmployedBee::ToggleFlowField()
{
	mDisplayFlowField = !mDisplayFlowField;
}

void EmployedBee::SetFlowFieldOctaveCount(const std::uint32_t& octaveCount)
{
	mFlowField = FlowFieldManager::GetInstance()->GetField();
	mFlowField.SetOctaveCount(octaveCount);
}

void EmployedBee::WaggleDance() const
{
	mParentHive.UpdateKnownFoodSource(mPairedFoodSource, mFoodSourceData);
	mParentHive.TriggerWaggleDance();
}

void EmployedBee::UpdateScouting(sf::RenderWindow& window, const float& deltaTime)
{
	UNREFERENCED_PARAMETER(window);

	//TODO: Remove this when metabolism can trigger giving up on scouting. for not. just give up if you go too far
	auto bounds = 5000;
	if (mPosition.x < mParentHive.GetCenterTarget().x - bounds || mPosition.x > mParentHive.GetCenterTarget().x + bounds || 
		mPosition.y < mParentHive.GetCenterTarget().y - bounds || mPosition.y > mParentHive.GetCenterTarget().y + bounds)
	{
		mState = State::DeliveringFood;
	}

	float rotationRadians = mFlowField.RadianValueAtPosition(mPosition);
	
	mVelocity.x += 2 * cos(rotationRadians);
	mVelocity.y += 2 * sin(rotationRadians);

	sf::Vector2f newPosition = mPosition + (mVelocity * deltaTime);

	auto foodSources = FoodSourceManager::GetInstance();
	for (auto iter = foodSources->Begin(); iter != foodSources->End(); ++iter)
	{
		if (DetectingFoodSource(*(*iter)) && !(*iter)->PairedWithEmployee())
		{
			mPairedFoodSource = (*iter);
			mTargetFoodSource = (*iter);
			mTargetFoodSource->SetPairedWithEmployee(true);
			SetTarget(mTargetFoodSource->GetCenterTarget());
			mHarvestingClock.restart();
			mState = State::HarvestingFood;
			break;
		}
	}

	UpdatePosition(newPosition, rotationRadians);
}

void EmployedBee::UpdateSeekingTarget(sf::RenderWindow& window, const float& deltaTime)
{
	UNREFERENCED_PARAMETER(window);
	auto facePosition = mFace.getPosition();
	float rotationRadians = atan2(mTarget.y - facePosition.y, mTarget.x - facePosition.x);
	auto newPosition = sf::Vector2f(
		mPosition.x + cos(rotationRadians) * mSpeed * deltaTime,
		mPosition.y + sin(rotationRadians) * mSpeed * deltaTime);

	UpdatePosition(newPosition, rotationRadians);

	if (DistanceBetween(newPosition, mTarget) <= TARGET_RADIUS)
	{
		mState = State::HarvestingFood;
		mHarvestingClock.restart();
	}
}

void EmployedBee::UpdateHarvestingFood(sf::RenderWindow& window, const float& deltaTime)
{
	UNREFERENCED_PARAMETER(window);

	auto facePosition = mFace.getPosition();
	float rotationRadians = atan2(mTarget.y - facePosition.y, mTarget.x - facePosition.x);
	auto newPosition = mPosition;

	if (mTargetFoodSource != nullptr)
	{
		if (DistanceBetween(mTarget, mPosition) <= TARGET_RADIUS)
		{
			auto dimensions = mTargetFoodSource->GetDimensions();
			uniform_int_distribution<int> distributionX(static_cast<int>(-dimensions.x / 2), static_cast<int>(dimensions.x / 2));
			uniform_int_distribution<int> distributionY(static_cast<int>(-dimensions.y / 2), static_cast<int>(dimensions.y / 2));
			sf::Vector2f offset(static_cast<float>(distributionX(mGenerator)), static_cast<float>(distributionY(mGenerator)));
			SetTarget(mTargetFoodSource->GetCenterTarget() + offset);
		}

		newPosition = sf::Vector2f(
			mPosition.x + cos(rotationRadians) * mSpeed * deltaTime,
			mPosition.y + sin(rotationRadians) * mSpeed * deltaTime);
	}

	mPosition = newPosition;

	if (mHarvestingClock.getElapsedTime().asSeconds() >= mHarvestingDuration)
	{
		HarvestFood(mTargetFoodSource->TakeFood(EXTRACTION_YIELD));
		if (mTargetFoodSource->GetFoodAmount() == 0.0f)
		{	// We just learned that the food source is no longer viable
			mAbandoningFoodSource = true;
		}
		mFoodSourceData.first = mTargetFoodSource->GetFoodAmount();
		mTargeting = false;
		mState = State::DeliveringFood;
	}

	SetColor(Bee::NORMAL_COLOR);

	auto foodSources = mCollisionNode->FoodSources();
	bool foodSourceFound = false;
	for (auto iter = foodSources.begin(); iter != foodSources.end(); ++iter)
	{
		if (CollidingWithFoodSource(*(*iter)))
		{
			foodSourceFound = true;
			SetColor(Bee::ALERT_COLOR);
			break;
		}
	}

	if (!foodSourceFound)
	{	// We need to search the neighbors now
		vector<CollisionNode*> neighbors = CollisionGrid::GetInstance()->NeighborsOf(mCollisionNode);
		for (int i = 0; i < neighbors.size(); i++)
		{
			auto neighborFoodSources = neighbors[i]->FoodSources();
			for (auto iter = neighborFoodSources.begin(); iter != neighborFoodSources.end(); ++iter)
			{
				foodSourceFound = true;
				SetColor(Bee::ALERT_COLOR);
				break;
			}
			if (foodSourceFound)
			{	// Early out if we find a collision
				break;
			}
		}
	}

	UpdatePosition(newPosition, rotationRadians);
}

void EmployedBee::UpdateDeliveringFood(sf::RenderWindow& window, const float& deltaTime)
{
	UNREFERENCED_PARAMETER(window);

	auto facePosition = mFace.getPosition();
	float rotationRadians = atan2(mTarget.y - facePosition.y, mTarget.x - facePosition.x);
	mTarget = mParentHive.GetCenterTarget();

	auto newPosition = sf::Vector2f(
		mPosition.x + cos(rotationRadians) * mSpeed * deltaTime,
		mPosition.y + sin(rotationRadians) * mSpeed * deltaTime);

	if (DistanceBetween(newPosition, mParentHive.GetCenterTarget()) <= TARGET_RADIUS)
	{
		mState = State::DepositingFood;
		mHarvestingClock.restart();
	}

	UpdatePosition(newPosition, rotationRadians);
}

void EmployedBee::UpdateDepositingFood(sf::RenderWindow& window, const float& deltaTime)
{
	UNREFERENCED_PARAMETER(window);

	auto facePosition = mFace.getPosition();
	float rotationRadians = atan2(mTarget.y - facePosition.y, mTarget.x - facePosition.x);

	if (DistanceBetween(mTarget, mPosition) <= TARGET_RADIUS)
	{
		auto dimensions = mParentHive.GetDimensions();
		uniform_int_distribution<int> distributionX(static_cast<int>(-dimensions.x / 2), static_cast<int>(dimensions.x / 2));
		uniform_int_distribution<int> distributionY(static_cast<int>(-dimensions.y / 2), static_cast<int>(dimensions.y / 2));
		sf::Vector2f offset(static_cast<float>(distributionX(mGenerator)), static_cast<float>(distributionY(mGenerator)));
		SetTarget(mParentHive.GetCenterTarget() + offset);
	}

	auto newPosition = sf::Vector2f(
		mPosition.x + cos(rotationRadians) * mSpeed * deltaTime,
		mPosition.y + sin(rotationRadians) * mSpeed * deltaTime);

	mPosition = newPosition;
	UpdatePosition(newPosition, rotationRadians);

	if (mHarvestingClock.getElapsedTime().asSeconds() >= mHarvestingDuration)
	{	// Now we go back to looking for another food source
		DepositFood(mFoodAmount);
		mTargeting = false;
		if (mPairedFoodSource != nullptr)
		{
			SetTarget(mPairedFoodSource->GetCenterTarget());
		}
		mState = (mPairedFoodSource == nullptr) ? State::Scouting : State::SeekingTarget;
		SetColor(Bee::NORMAL_COLOR);
		WaggleDance();

		if (mAbandoningFoodSource)
		{	// If food source is marked for abandon, we forget about it and tell the hive to forget about it
			mParentHive.RemoveFoodSource(mPairedFoodSource);
			if (mPairedFoodSource != nullptr)
			{
				mPairedFoodSource->SetPairedWithEmployee(false);
			}
			mPairedFoodSource = nullptr;
			mAbandoningFoodSource = false;
		}
	}

	SetColor(Bee::NORMAL_COLOR);

	auto foodSources = mCollisionNode->FoodSources();
	bool foodSourceFound = false;
	for (auto iter = foodSources.begin(); iter != foodSources.end(); ++iter)
	{
		if (CollidingWithFoodSource(*(*iter)))
		{
			foodSourceFound = true;
			SetColor(Bee::ALERT_COLOR);
			break;
		}
	}

	if (!foodSourceFound)
	{
		vector<CollisionNode*> neighbors = CollisionGrid::GetInstance()->NeighborsOf(mCollisionNode);
		for (int i = 0; i < neighbors.size(); i++)
		{
			auto neighborFoodSources = neighbors[i]->FoodSources();
			for (auto iter = neighborFoodSources.begin(); iter != neighborFoodSources.end(); ++iter)
			{
				if (CollidingWithFoodSource(*(*iter)))
				{
					foodSourceFound = true;
					SetColor(Bee::ALERT_COLOR);
					break;
				}
			}
			if (foodSourceFound)
			{	// Early out if we found a collision
				break;
			}
		}
	}

	if (CollidingWithHive(mParentHive))
	{
		SetColor(Bee::ALERT_COLOR);
	}
}

void EmployedBee::UpdatePosition(const sf::Vector2f& position, const float& rotation)
{
	DetectStructureCollisions();
	mPosition = position;
	mBody.setPosition(sf::Vector2f(mPosition.x - BodyRadius, mPosition.y - BodyRadius));
	mFace.setPosition(mPosition.x, mPosition.y);
	auto rotationAngle = rotation * (180.0f / PI);
	mFace.setRotation(rotationAngle);
}

void EmployedBee::UpdateFlowField(sf::RenderWindow& window, const float& deltaTime)
{
	auto fieldDimensions = mFlowField.GetDimensions();
	if (!mFlowField.CollidingWith(mPosition))
	{	// If we're not colliding with the flow field anymore, reset it on top of us
		mFlowField = FlowFieldManager::GetInstance()->GetField();
		mFlowField.SetPosition(sf::Vector2f(mPosition.x - (fieldDimensions.x / 2.0f), mPosition.y - (fieldDimensions.y / 2.0f)));
	}
	mFlowField.Update(window, deltaTime);
}