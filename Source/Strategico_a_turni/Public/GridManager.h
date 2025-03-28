// Fill out your copyright notice in the Description page of Project Settings.

//GridManager
//Manages a grid - based game board with tiles, pathfinding capabilities, and obstacle generation.
//Controls tile visualization, unit movement, and grid connectivity.

#pragma once

#include "CoreMinimal.h"
#include "Tile.h"
#include "GameFramework/Actor.h"
#include "GridManager.generated.h"

class AUnit;

UCLASS()
class STRATEGICO_A_TURNI_API AGridManager : public AActor
{

	GENERATED_BODY()

public:
    // Constants
    static const int32 NOT_ASSIGNED = -1;

    // ----------------------------------------
    // Constructors and lifecycle methods
    // ----------------------------------------

    /** Sets default values for this actor's properties */
    AGridManager();

    /** Called when an instance of this class is placed (in editor) or spawned */
    virtual void OnConstruction(const FTransform& Transform) override;

    /** Called when the game starts or when spawned */
    virtual void BeginPlay() override;

    // ----------------------------------------
    // Grid generation and setup
    // ----------------------------------------

    /** Generates an empty game field with the specified size */
    void GenerateField();

    /** Generates obstacles randomly throughout the grid based on ObstaclePercentage */
    UFUNCTION(BlueprintCallable, Category = "Grid")
    void GenerateObstacles();

    /** Ensures all accessible areas of the grid are connected to each other */
    UFUNCTION(BlueprintCallable, Category = "Grid")
    bool EnsureGridConnectivity();

    // ----------------------------------------
    // Grid interaction and query methods
    // ----------------------------------------

    /** Returns the grid position corresponding to a hit result (e.g., from a click) */
    FVector2D GetPosition(const FHitResult& Hit);

    /** Returns the array of all tiles in the grid */
    TArray<ATile*>& GetTileArray();

    /** Converts grid coordinates to a world location */
    UFUNCTION(BlueprintCallable, Category = "Grid")
    FVector GetWorldLocationFromGrid(int32 GridX, int32 GridY);

    /** Returns true if the specified cell is occupied by a unit or obstacle */
    UFUNCTION(BlueprintCallable, Category = "Grid")
    bool IsCellOccupied(int32 GridX, int32 GridY);

    /** Places a unit at the specified grid coordinates. Pass nullptr to clear the cell. */
    UFUNCTION(BlueprintCallable, Category = "Grid")
    void OccupyCell(int32 GridX, int32 GridY, AUnit* Unit);

    /** Returns the world location for the given grid coordinates */
    FVector GetRelativeLocationByXYPosition(const int32 InX, const int32 InY) const;

    /** Returns the grid coordinates for the given world location */
    FVector2D GetXYPositionByRelativeLocation(const FVector& Location) const;

    /** Checks if the given position is within the grid boundaries */
    inline bool IsValidPosition(const FVector2D Position) const;

    // ----------------------------------------
    // Visualization and highlighting methods
    // ----------------------------------------

    /** Highlights or un-highlights a specific cell */
    UFUNCTION(BlueprintCallable, Category = "Grid")
    void HighlightCell(int32 GridX, int32 GridY, bool bHighlight);

    /** Removes highlighting from all cells */
    UFUNCTION(BlueprintCallable, Category = "Grid")
    void ClearAllHighlights();

    /** Highlights a path between multiple points */
    UFUNCTION(BlueprintCallable, Category = "Grid")
    bool HighlightPath(TArray<FVector2D> PathPoints, bool bClearPrevious = true);

    /** Removes highlighting from path cells */
    UFUNCTION(BlueprintCallable, Category = "Grid")
    void ClearPathHighlights();

    // ----------------------------------------
    // Utility and debug methods
    // ----------------------------------------

    /** Prints debug information about loaded materials */
    void DebugMaterials();

    /** Prints debug information about obstacles */
    UFUNCTION(BlueprintCallable, Category = "Grid")
    void DebugObstacles();

    /** Converts grid coordinates to letter-number format (e.g., B4) */
    UFUNCTION(BlueprintCallable, Category = "Grid")
    // Convert grid coordinates to letter-number format (e.g., B4)
    static FString ConvertToLetterNumberFormat(int32 GridX, int32 GridY)
    {
        // Convert Y to a letter (A-Y)
        TCHAR Letter = 'A' + GridY;

        // X is already a number (1-25)
        int32 Number = GridX + 1;

        // Return in "Letter+Number" format
        return FString::Printf(TEXT("%c%d"), Letter, Number);
    }

    /** Converts letter-number format back to grid coordinates */
    UFUNCTION(BlueprintCallable, Category = "Grid")
    static bool ConvertFromLetterNumberFormat(const FString& Coordinate, int32& OutGridX, int32& OutGridY)
    {
        if (Coordinate.Len() < 2)
            return false;

        // First character is the letter (Y coordinate)
        TCHAR Letter = Coordinate[0];
        if (Letter >= 'A' && Letter <= 'Y')
        {
            OutGridY = Letter - 'A';
        }
        else if (Letter >= 'a' && Letter <= 'y')
        {
            OutGridY = Letter - 'a';
        }
        else
        {
            return false;
        }

        // Rest of the string should be the number (X coordinate)
        FString NumberStr = Coordinate.Mid(1);
        int32 Number = FCString::Atoi(*NumberStr);

        // Check if number is in valid range
        if (Number >= 1 && Number <= 25)
        {
            OutGridX = Number - 1;
            return true;
        }

        return false;
    }

    /** Gets a line of tile owners between two positions */
    TArray<int32> GetLine(const FVector2D Begin, const FVector2D End) const;

    /** Checks if all elements in an array are equal */
    bool AllEqual(const TArray<int32>& Array) const;

    // ----------------------------------------
    // Public properties
    // ----------------------------------------

    /** Size of the grid (will be Size x Size) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grid")
    int32 Size;

    /** Multiplier for tile positioning */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Grid")
    float NextCellPositionMultiplier;

    /** Padding between tiles as a percentage */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grid")
    float CellPadding;

    /** Size of each tile in world units */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grid")
    float TileSize;

    /** Percentage of the grid to fill with obstacles (0.0 to 1.0) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Grid", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float ObstaclePercentage;

    // ----------------------------------------
    // Materials
    // ----------------------------------------

    /** Default material for tiles */
    UPROPERTY(EditDefaultsOnly, Category = "Materials")
    UMaterialInterface* DefaultTileMaterial;

    /** Material used for highlighting selected tiles */
    UPROPERTY(EditDefaultsOnly, Category = "Materials")
    UMaterialInterface* HighlightMaterial;

    /** Material used for showing valid movement paths */
    UPROPERTY(EditDefaultsOnly, Category = "Materials")
    UMaterialInterface* PathMaterial;

    /** Material used for obstacles */
    UPROPERTY(EditDefaultsOnly, Category = "Materials")
    UMaterialInterface* ObstacleMaterial;

    // ----------------------------------------
    // Grid connectivity methods
    // ----------------------------------------

    /** Identifies separate regions in the grid */
    int32 IdentifyRegions(const TArray<bool>& ObstacleMap, TArray<int32>& RegionMap);

    /** Flood fills a region starting from the given coordinates */
    void FloodFillRegion(
        const TArray<bool>& ObstacleMap,
        TArray<int32>& RegionMap,
        TArray<bool>& Visited,
        int32 StartX, int32 StartY,
        int32 RegionId
    );

    /** Finds the largest connected region in the grid */
    int32 FindLargestRegion(const TArray<int32>& RegionMap, int32 RegionCount);

    /** Connects separated regions by removing obstacles */
    bool ConnectRegions(
        TArray<bool>& ObstacleMap,
        const TArray<int32>& RegionMap,
        int32 LargestRegionId
    );

    /** Removes obstacles to create a path to an unreachable cell */
    bool RemoveObstacleToCreatePath(int32 UnreachableX, int32 UnreachableY);

    // ----------------------------------------
    // Protected properties
    // ----------------------------------------

    /** Class to use when spawning tiles */
    UPROPERTY(EditDefaultsOnly, Category = "Grid")
    TSubclassOf<ATile> TileClass;

    /** Static mesh component for the grid */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    UStaticMeshComponent* StaticMeshComponent;

    /** Keeps track of all tiles in the grid */
    UPROPERTY(Transient)
    TArray<ATile*> TileArray;

    /** Maps grid positions to tile objects */
    UPROPERTY(Transient)
    TMap<FVector2D, ATile*> TileMap;

    /** Tracks which tiles are currently highlighted */
    UPROPERTY()
    TArray<ATile*> HighlightedTiles;

    /** Tracks which tiles are part of the highlighted path */
    UPROPERTY()
    TArray<ATile*> PathTiles;

};