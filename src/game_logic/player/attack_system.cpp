/* Copyright (C) 2016, Nikolai Wuttke. All rights reserved.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "attack_system.hpp"

#include "game_mode.hpp"


namespace rigel { namespace game_logic { namespace player {

using engine::components::WorldPosition;

namespace ex = entityx;

namespace {

const auto DUKE_SHOT_VELOCITY = 2.0f;

}


AttackSystem::AttackSystem(
  entityx::Entity playerEntity,
  IGameServiceProvider* pServiceProvider,
  FireShotFunc fireShotFunc
)
  : mPlayerEntity(playerEntity)
  , mpServiceProvider(pServiceProvider)
  , mFireShotFunc(fireShotFunc)
{
}


void AttackSystem::setInputState(PlayerInputState inputState) {
  mInputState = inputState;
}


void AttackSystem::update(
  ex::EntityManager& es,
  ex::EventManager& events,
  ex::TimeDelta dt
) {
  assert(mPlayerEntity.has_component<components::PlayerControlled>());
  assert(mPlayerEntity.has_component<WorldPosition>());

  auto& playerState =
    *mPlayerEntity.component<components::PlayerControlled>().get();
  const auto& playerPosition =
    *mPlayerEntity.component<WorldPosition>().get();

  if (
    playerState.mState == PlayerState::ClimbingLadder ||
    playerState.mState == PlayerState::Dieing ||
    playerState.mState == PlayerState::Dead
  ) {
    return;
  }

  if (!mInputState.mShooting) {
    playerState.mShotFired = false;
    return;
  }

  const auto canShoot = !playerState.mShotFired;
  if (canShoot) {
    fireShot(playerPosition, playerState);
    playerState.mShotFired = true;
  }
}


void AttackSystem::fireShot(
  const WorldPosition& playerPosition,
  const components::PlayerControlled& playerState
) {
  const auto facingRight =
    playerState.mOrientation == player::Orientation::Right;
  const auto state = playerState.mState;

  const auto shotOffsetHorizontal = state == PlayerState::LookingUp
    ? (facingRight ? 2 : 0)
    : (facingRight ? 3 : -1);
  const auto shotOffsetVertical = state != PlayerState::Crouching
    ? (state == PlayerState::LookingUp ? -5 : -2)
    : -1;

  const auto shotOffset =
    WorldPosition{shotOffsetHorizontal, shotOffsetVertical};

  const auto directionAdjustment = state != PlayerState::LookingUp
    ? (facingRight ? 1.0f : -1.0f)
    : -1.0f;
  const auto velocity = DUKE_SHOT_VELOCITY * directionAdjustment;

  const auto velocityVector = state == PlayerState::LookingUp
    ? base::Point<float>{0.0f, velocity}
    : base::Point<float>{velocity, 0.0f};

  mFireShotFunc(shotOffset + playerPosition, velocityVector);
  mpServiceProvider->playSound(data::SoundId::DukeNormalShot);
}

}}}