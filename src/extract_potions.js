const cheerio = require('cheerio');
const request = require('request-promise');
const yargs = require('yargs');
const fs = require('fs');


const local_path = './presaved/tesv_ingredients.html';
const uesp_link = 'https://en.uesp.net/wiki/Skyrim:Ingredients';

let argv = yargs.argv;
console.log(argv);


//Skill Tree is described as points invested in following order - 
//Alchemist (1-5), Physician, Benefactor, Experimenter(1-3), Poisoner, Concentrated Poison, Green Thumb, Snakeblood, Purity
let skillTree = [ 5, 1, 1, 3, 1, 1, 1, 1, 1, 1];//maximum points invested example, to be changed for a configurable reads

let skillLevel = 100;
async function main () {
	
	let Skill = int(skillLevel),
		fAlchemyIngredientInitMult = 4,
		fAlchemySkillFactor = 1.5,
		BaseMag = NaN//TBD,
		SkillMult = 1 + (fAlchemySkillFactor-1) * Skill / 100,
	let Result = fAlchemyIngredientInitMult * BaseMag *  SkillMult * Al
}