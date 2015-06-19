// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

/**
 * Instance of a UMovieSceneFloatTrack
 */
class FMovieSceneFloatTrackInstance : public IMovieSceneTrackInstance
{
public:
	FMovieSceneFloatTrackInstance( UMovieSceneFloatTrack& InFloatTrack );

	/** IMovieSceneTrackInstance interface */
	virtual void SaveState (const TArray<UObject*>& RuntimeObjects) override;
	virtual void RestoreState (const TArray<UObject*>& RuntimeObjects) override;
	virtual void Update( float Position, float LastPosition, const TArray<UObject*>& RuntimeObjects, class IMovieScenePlayer& Player ) override;
	virtual void RefreshInstance( const TArray<UObject*>& RuntimeObjects, class IMovieScenePlayer& Player ) override;
private:
	/** Track that is being instanced */
	UMovieSceneFloatTrack* FloatTrack;
	/** Runtime property bindings */
	TSharedPtr<class FTrackInstancePropertyBindings> PropertyBindings;
	/** Map from object to initial state */
	TMap< TWeakObjectPtr<UObject>, float > InitFloatMap;
};
