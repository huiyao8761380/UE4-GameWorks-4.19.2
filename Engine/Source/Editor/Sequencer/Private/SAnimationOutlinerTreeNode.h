// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

#pragma once

class SSequencerTreeView;
class SSequencerTreeViewRow;

/**
 * A widget for displaying a sequencer tree node in the animation outliner
 */
class SAnimationOutlinerTreeNode : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SAnimationOutlinerTreeNode){}
	SLATE_END_ARGS()

	void Construct( const FArguments& InArgs, TSharedRef<FSequencerDisplayNode> Node, const TSharedRef<SSequencerTreeViewRow>& InTableRow );

	/**
	 * @return The display node used by this widget                                                              
	 */
	const TSharedPtr< FSequencerDisplayNode> GetDisplayNode() const { return DisplayNode; }

	/**
	 * Gets a tint to apply buttons on hover
	 */
	FLinearColor GetHoverTint() const;

private:
	/** SWidget Interface */
	virtual FReply OnMouseButtonDown( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent ) override;
	virtual FReply OnMouseButtonUp( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent ) override;

	FSlateColor GetForegroundBasedOnSelection() const;

	/**
	 * @return The border image to show in the tree node
	 */
	const FSlateBrush* GetNodeBorderImage() const;
	
	/**
	 * @return The expander visibility of this node
	 */
	EVisibility GetExpanderVisibility() const;

	/**
	 * @return The display name for this node.
	 */
	FText GetDisplayName() const;

	/** Handles the previous key button being clicked. */
	FReply OnPreviousKeyClicked();

	/** Handles the next key button being clicked. */
	FReply OnNextKeyClicked();

	/** Handles the add key button being clicked. */
	FReply OnAddKeyClicked();

	/** Get all descendant nodes from the given root node */
	void GetAllDescendantNodes(TSharedPtr<FSequencerDisplayNode> RootNode, TArray<TSharedRef<FSequencerDisplayNode> >& AllNodes);

	void OnSelectionChanged( TArray<TSharedPtr<FSequencerDisplayNode> > AffectedNodes );

private:
	/** Layout node the widget is visualizing */
	TSharedPtr<FSequencerDisplayNode> DisplayNode;
	/** Brush to display a border around the widget when it is selected */
	const FSlateBrush* SelectedBrush;
	/** Brush to display a border around the widget when it is selected but inactive */
	const FSlateBrush* SelectedBrushInactive;
	/** Brush to use if the node is not selected */
	const FSlateBrush* NotSelectedBrush;
	/** The table row style used for nodes in the tree. This is required as we don't actually use the tree for selection */
	const FTableRowStyle* TableRowStyle;
};