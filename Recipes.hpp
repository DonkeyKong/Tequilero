#ifndef RECIPES_HPP
#define RECIPES_HPP

#include <string>
#include <vector>

struct Ingredient
{
  std::string name;
  float abv;
};

struct RecipeIngredient
{
  std::string name;
  int ratio;
};

struct Recipe
{
  std::string name;
  std::vector<RecipeIngredient> ingredients;
  std::string procedure;
};

struct Settings
{
  std::vector<Ingredient> ingredients;
  std::vector<std::string> pumps;
  std::vector<Recipe> recipes;
};

#endif // RECIPES_HPP
