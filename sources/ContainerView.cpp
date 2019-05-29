#include "ContainerView.h"
#include "Log.h"
#include "Lexer.h"
#include "Entity.h"
#include "Container.h"
#include "Variable.h"
#include "BinaryOperation.h"
#include "Wire.h"
#include "WireView.h"
#include "DataAccess.h"
#include <cstring>      // for strcmp
#include <algorithm>    // for std::find_if
#include "NodeView.h"
#include "Application.h"
#include <IconFontCppHeaders/IconsFontAwesome5.h>

using namespace Nodable;

bool ContainerView::draw()
{
	
	auto origin = ImGui::GetCursorScreenPos();
	auto entities = this->getOwner()->getAs<Container*>()->getEntities();

	/*
		NodeViews
	*/
	NodeView::currentMemberHoveredByMouse = nullptr; // reset this var befor drawing
	bool isAnyNodeDragged = false;
	bool isAnyNodeHovered = false;
	{
		// Update
		for (auto eachNode : entities)
		{
			if (eachNode->hasComponent("view"))
				eachNode->getComponent("view")->update();
		}

		//  Draw

		for (auto eachNode : entities)
		{
			if (eachNode->hasComponent("view"))
			{
				auto view = eachNode->getComponent("view")->getAs<View*>();

				if (view != nullptr && view->isVisible())
				{
					view->draw();
					isAnyNodeDragged |= NodeView::GetDragged() == view;
					isAnyNodeHovered |= view->isHovered();
				}
			}
		}
	}
	/*
		Wires
	*/
	{
		// Draw existing wires
		for (auto eachNode : entities)
		{
			auto wires = eachNode->getWires();

			for (auto eachWire : wires)
			{
				if (eachWire->getTarget()->getOwner() == eachNode)
					eachWire->getView()->draw();
			}
		}

		// Draw temporary wire on top (overlay draw list)
		if (NodeView::lastMemberDraggedByMouse != nullptr)
		{
			auto lineStartPosition = NodeView::lastMemberDraggedByMouse->getOwner()->getAs<Entity*>()->getComponent("view")->getAs<NodeView*>()->getInputPosition(NodeView::lastMemberDraggedByMouse->getName()) + ImGui::GetWindowPos();
			auto lineEndPosition = ImGui::GetMousePos();

			// Snap lineEndPosition to hovered member position
			if (NodeView::currentMemberHoveredByMouse != nullptr)
				lineEndPosition = NodeView::currentMemberHoveredByMouse->getOwner()->getAs<Entity*>()->getComponent("view")->getAs<NodeView*>()->getInputPosition(NodeView::currentMemberHoveredByMouse->getName()) + ImGui::GetWindowPos();
			
			ImGui::GetOverlayDrawList()->AddLine(lineStartPosition, lineEndPosition, getColor(ColorType_BorderHighlights), connectorRadius * float(0.9));
		}

		// Add a new wire if needed (mouse drag'n drop)
		if (NodeView::lastMemberDraggedByMouse           != nullptr &&
			NodeView::lastMemberHoveredWhenMouseReleased != nullptr)
		{
			if (NodeView::lastMemberDraggedByMouse != NodeView::lastMemberHoveredWhenMouseReleased)
			{
				auto wire = NodeView::lastMemberDraggedByMouse->getOwner()->getAs<Entity*>()->getParent()->createWire();
				Entity::Connect(wire, NodeView::lastMemberDraggedByMouse, NodeView::lastMemberHoveredWhenMouseReleased);
			}

			NodeView::lastMemberDraggedByMouse           = nullptr;
			NodeView::lastMemberHoveredWhenMouseReleased = nullptr;
		}

		// Mouse inputs relative to temporary wire
		else if (!ImGui::IsMouseReleased(0) && !ImGui::IsMouseDown(0))
		{
			NodeView::lastMemberDraggedByMouse           = nullptr;
			NodeView::lastMemberHoveredWhenMouseReleased = nullptr;
		}
	}

	/*
		Deselection
	*/
	// Deselection
	if (NodeView::GetSelected() != nullptr && !isAnyNodeHovered && ImGui::IsMouseClicked(0) && ImGui::IsWindowFocused())
		NodeView::SetSelected(nullptr);

	/*
		Mouse PAN (global)
	*/

	bool isMousePanEnable = false;
	if (isMousePanEnable)
	{	if (ImGui::IsMouseDragging() && ImGui::IsWindowFocused() && !isAnyNodeDragged)
		{
			auto drag = ImGui::GetMouseDragDelta();
			for (auto eachNode : entities)
			{
				((NodeView*)eachNode->getComponent("view"))->translate(drag);
			}
			ImGui::ResetMouseDragDelta();
		}
	}

	/*
		Mouse right-click popup menu
	*/

	if (ImGui::IsMouseClicked(1))
		ImGui::OpenPopup("ContainerViewContextualMenu");

	if (ImGui::BeginPopup("ContainerViewContextualMenu"))
	{
		auto container = getOwner()->getAs<Container*>();

		Entity* newEntity = nullptr;

		if ( ImGui::BeginMenu("New operation")){
			
			if (ImGui::MenuItem("Add"))
				newEntity = container->createNodeAdd();

			if (ImGui::MenuItem("Divide"))
				newEntity = container->createNodeDivide();

			if (ImGui::MenuItem("Multiply"))
				newEntity = container->createNodeMultiply();

			if (ImGui::MenuItem("Substract"))
				newEntity = container->createNodeSubstract();

			ImGui::EndMenu();
		}

		if (ImGui::MenuItem("New variable"))
			newEntity = container->createNodeVariable("Variable");

		if (ImGui::MenuItem("New result"))
			newEntity = container->createNodeResult();

		/*
			Set New Entity's position were mouse cursor is 
		*/

		if (newEntity != nullptr && newEntity->hasComponent("view"))
		{
			auto view = static_cast<NodeView*>(newEntity->getComponent("view"));

			if (view != nullptr)
			{
				auto pos = ImGui::GetMousePos();
				pos.x -= origin.x;
				pos.y -= origin.y;
				view->setPosition(pos);
			}
			
		}

		ImGui::EndPopup();

	}

	return true;
}