#include "Actor.h"

bool Actor::operator==(Actor AnotherActor)
{
	return this->ActorType == AnotherActor.ActorType &&
		this->Gyml == AnotherActor.Gyml &&
		this->SRTHash == AnotherActor.SRTHash &&
		this->Hash == AnotherActor.Hash &&
		this->Translate == AnotherActor.Translate &&
		this->Rotate == AnotherActor.Rotate &&
		this->Scale == AnotherActor.Scale;
}