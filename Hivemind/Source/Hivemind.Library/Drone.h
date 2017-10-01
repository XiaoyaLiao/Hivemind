#pragma once
#include "Bee.h"
#include <SFML/System/Vector2.hpp>


class Drone : public Bee
{
public:

	/**
	 * Constructor
	 * @Param position: The starting position of the bee
	 * @Param hive: The hive that this bee belongs to
	 */
	explicit Drone(const sf::Vector2f& position, Hive& hive);

	/**
	 *  Destructor
	 */
	virtual ~Drone();

	/**
	 * Update method called by the main game loop
	 * @Param window: The window that the simulation is being rendered to
	 * @Param deltaTime: The time since the last Update call
	 */
	void Update(sf::RenderWindow& window, const double& deltaTime) override;

	/**
	 * Render method called by the main game loop
	 * @Param window: The window that the simulation is being rendered to
	 */
	void Render(sf::RenderWindow& window) const override;

protected:

	// Drones don't have state so we don't need to use the function map
	virtual void PopulateFunctionMaps() override {}

private:

	/**
	 * Determines if any comb is needed and constructrs/converts as much as it can
	 * @Param deltaTime: Time since last update call was made
	 */
	void HandleCombManagement(const float& deltaTime) const;

};

