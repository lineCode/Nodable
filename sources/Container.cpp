#include "Container.h"
#include "Log.h"
#include "Parser.h"
#include "Entity.h"
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

ImVec2 Container::LastResultNodePosition = ImVec2(-1, -1); // draft try to store node position

Container::~Container()
{
	clear();
}

void Container::clear()
{
	// Store the Result node position to restore it later
	if (result != nullptr) {
		auto view = result->getComponent<NodeView>("view");
		Container::LastResultNodePosition = view->getRoundedPosition();
	}

	
	for (Entity* each : entities)
		delete each;
	
	entities.resize(0);
	variables.resize(0);
	result = nullptr;
}

bool Container::update()
{
	// Update entities
	size_t entitiesUpdated(0);

	for(auto it = entities.begin(); it < entities.end(); ++it)
	{
		auto entity = *it;

		if ( entity )
		{
			if ( entity->needsToBeDeleted())
			{
				delete entity;
				it = entities.erase(it);
			}
			else if ( entity->isDirty())
			{
				entitiesUpdated++;
				entity->update();
			}
		}
	}

	const bool hasChanged = entitiesUpdated > 0 && NodeView::GetSelected() != nullptr;

	return hasChanged;
}

void Container::addEntity(Entity* _entity)
{
	/* Add the node to the node vector list */
	this->entities.push_back(_entity);

	/* Set the node's container to this */
	_entity->setParent(this);

}

void Container::destroyNode(Entity* _entity)
{
	{
		auto it = std::find(variables.begin(), variables.end(), _entity);
		if (it != variables.end())
			variables.erase(it);
	}

	{
		auto it = std::find(entities.begin(), entities.end(), _entity);
		if (it != entities.end())
			entities.erase(it);
	}

	delete _entity;
}

Variable* Container::find(std::string _name)
{
	Variable* result = nullptr;

	auto findFunction = [_name](const Variable* _variable ) -> bool
	{
		return strcmp(_variable->getName(), _name.c_str()) == 0;
	};

	auto it = std::find_if(variables.begin(), variables.end(), findFunction);
	if (it != variables.end()){
		result = *it;
	}

	return result;
}

Variable* Container::createNodeResult()
{
	auto variable = createNodeVariable(ICON_FA_SIGN_OUT_ALT " Result");
	auto member = variable->getMember("value");
	member->setConnectionFlags(Connection_In);                     // disable output because THIS node is the output !
	result = variable;

	return variable;
}

Variable* Container::createNodeVariable(std::string _name)
{
	auto variable = new Variable();
	variable->addComponent( "view", new NodeView);
	variable->setName(_name.c_str());
	this->variables.push_back(variable);
	this->addEntity(variable);
	return variable;
}

Variable* Container::createNodeNumber(double _value)
{
	auto node = new Variable();
	node->addComponent( "view", new NodeView);
	node->setValue(_value);
	this->addEntity(node);
	return node;
}

Variable* Container::createNodeNumber(const char* _value)
{
	auto node = new Variable();
	node->addComponent( "view", new NodeView);
	node->setValue(std::stod(_value));
	this->addEntity(node);
	return node;
}

Variable* Container::createNodeString(const char* _value)
{
	auto node = new Variable();
	node->addComponent( "view", new NodeView);
	node->setValue(_value);
	this->addEntity(node);
	return node;
}


Entity* Container::createNodeBinaryOperation(std::string _op)
{
	Entity* node;

	if (_op == "+")
		node = createNodeAdd();
	else if (_op == "-")
		node = createNodeSubstract();
	else if (_op == "*")
		node = createNodeMultiply();
	else if (_op == "/")
		node = createNodeDivide();
	else
		node = nullptr;
	
	return node;
}


Entity* Container::createNodeAdd()
{
	// Create a node with 2 inputs and 1 output
	auto node 		= new Entity();	
	node->setLabel(ICON_FA_PLUS " Add");
	node->addMember("left",   Visibility_Default, Type_Number, Connection_In);
	node->addMember("right",  Visibility_Default, Type_Number, Connection_In);
	node->addMember("result", Visibility_Default, Type_Number, Connection_Out);
	
	// Create a binary operation component and link values.
	auto operation 	= new Add();
	operation->setLeft  (node->getMember("left"));
	operation->setRight (node->getMember("right"));
	operation->setResult(node->getMember("result"));
	operation->setOperatorAsString("+");
	node->addComponent( "operation", operation);
	
	// Create a view component
	node->addComponent( "view", new NodeView);

	this->addEntity(node);

	return node;
}

Entity* Container::createNodeSubstract()
{
	// Create a node with 2 inputs and 1 output
	auto node 		= new Entity();	
	node->setLabel(ICON_FA_MINUS " Sub");
	node->addMember("left",   Visibility_Default, Type_Number, Connection_In);
	node->addMember("right",  Visibility_Default, Type_Number, Connection_In);
	node->addMember("result", Visibility_Default, Type_Number, Connection_Out);

	// Create a binary operation component and link values.
	auto operation 	= new Substract();
	operation->setLeft  (node->getMember("left"));
	operation->setRight (node->getMember("right"));
	operation->setResult(node->getMember("result"));
	operation->setOperatorAsString("-");
	node->addComponent( "operation", operation);

	// Create a view component
	node->addComponent( "view", new NodeView);

	this->addEntity(node);

	return node;
}

Entity* Container::createNodeMultiply()
{
	// Create a node with 2 inputs and 1 output
	auto node 		= new Entity();	
	node->setLabel(ICON_FA_TIMES " Mult");
	node->addMember("left",   Visibility_Default, Type_Number, Connection_In);
	node->addMember("right",  Visibility_Default, Type_Number, Connection_In);
	node->addMember("result", Visibility_Default, Type_Number, Connection_Out);

	// Create a binary operation component and link values.
	auto operation 	= new Multiply();
	operation->setLeft  (node->getMember("left"));
	operation->setRight (node->getMember("right"));
	operation->setResult(node->getMember("result"));
	operation->setOperatorAsString("*");

	node->addComponent( "operation", operation);

	// Create a view component
	node->addComponent( "view", new NodeView);

	this->addEntity(node);

	return node;
}

Entity* Container::createNodeDivide()
{
	// Create a node with 2 inputs and 1 output
	auto node 		= new Entity();	
	node->setLabel(ICON_FA_DIVIDE " Div");
	node->addMember("left",   Visibility_Default, Type_Number, Connection_In);
	node->addMember("right",  Visibility_Default, Type_Number, Connection_In);
	node->addMember("result", Visibility_Default, Type_Number, Connection_Out);

	// Create a binary operation component and link values.
	auto operation 	= new Divide();
	operation->setLeft  (node->getMember("left"));
	operation->setRight (node->getMember("right"));
	operation->setResult(node->getMember("result"));
	operation->setOperatorAsString("/");

	node->addComponent( "operation", operation);

	// Create a view component
	node->addComponent( "view", new NodeView);

	this->addEntity(node);

	return node;
}

Entity* Container::createNodeAssign()
{
	// Create a node with 2 inputs and 1 output
	auto node 		= new Entity();	
	node->setLabel("ASSIGN");
	node->addMember("left",   Visibility_Default, Type_Number, Connection_In);
	node->addMember("right",  Visibility_Default, Type_Number, Connection_In);
	node->addMember("result", Visibility_Default, Type_Number, Connection_Out);

	// Create a binary operation component and link values.
	auto operation 	= new Assign();
	operation->setLeft  (node->getMember("left"));
	operation->setRight (node->getMember("right"));
	operation->setResult(node->getMember("result"));
	operation->setOperatorAsString("=");

	node->addComponent( "operation", operation);

	// Create a view component
	node->addComponent( "view", new NodeView);

	this->addEntity(node);

	return node;
}

Wire* Container::createWire()
{
	Wire* wire = new Wire;
	wire->addComponent("view", new WireView);	
	this->addEntity(wire);
	return wire;
}

void Container::tryToRestoreResultNodePosition()
{
	// Store the Result node position to restore it later
	auto nodeView = result->getComponent<NodeView>("view");	
	bool resultNodeHadPosition = Container::LastResultNodePosition.x != -1 &&
	                             Container::LastResultNodePosition.y != -1;

	if (nodeView && this->hasComponent("view") ) {

		auto view = this->getComponent<View>("view");

		if ( resultNodeHadPosition) {                                 /* if result node had a position stored, we restore it */
			nodeView->setPosition(Container::LastResultNodePosition);
			NodeView::ConstraintToRect(nodeView, view->visibleRect);   // but we constraint it to be visible

		} else {                                                      /* else we set a default position*/			
			ImVec2 defaultPosition = view->visibleRect.GetCenter();
			defaultPosition.x += view->visibleRect.GetWidth() * 1.0f / 6.0f;
			nodeView->setPosition(defaultPosition);
		}
	}
}

Parser* Container::createNodeParser(Variable* _expressionVariable)
{
	// Create a Parser Node
	auto language = Language::NODABLE;
	Parser* node = new Parser( language );
	node->setLabel(ICON_FA_COGS " Parser");
	
	// Attach a NodeView on it
	auto view = new NodeView;
	view->setVisible(false);
	node->addComponent( "view", view);	

	// Link the _expressionVariable output with the Parser's member "expression"
	auto wire             = this->createWire();
	auto expressionMember = node->getMember("expression");
	Entity::Connect(wire,_expressionVariable->getValueMember(), expressionMember);
	expressionMember->updateValueFromInputMemberValue();

	this->addEntity(node);
	return node;
}

size_t Container::getSize()const
{
	return entities.size();
}