const cheerio = require('cheerio');
const request = require('request-promise');
const yargs = require('yargs');
const fs = require('fs');


const local_path = './presaved/tesv_ingredients.html';
const uesp_link = 'https://en.uesp.net/wiki/Skyrim:Ingredients';

let argv = yargs.argv;
console.log(argv);


async function main() {

    if (argv._.includes('online')) {

        console.log("Pulling HTML page from Internet!");

        const result = await request.get(uesp_link);

        $ = cheerio.load(result);

    } else {

        console.log("Loading presaved HTML page");
        $ = cheerio.load(fs.readFileSync(local_path));

    }


    if (!fs.existsSync('./data/img')) {
        fs.mkdirSync('./data/img', {
            recursive: true
        });
    }


    const Ingredients = new Array;
    

    console.log('Extracting base game ingredients...');

    for (let i = 2; i <= 400; i += 2) { //for main table table[14], for CC - table[20]
        let sel = new Array;
        sel[0] = `#mw-content-text > table:nth-child(14) > tbody > tr:nth-child(${i}) > td:nth-child(2) > a`; //name

        if ($(sel[0]) == "")
            break;

        let mag = new Array(4).fill(null),
            dur = new Array(4).fill(null),
            val = new Array(4).fill(null);

        sel[1] = `#mw-content-text > table:nth-child(14) > tbody > tr:nth-child(${i}) > td:nth-child(3)`; //where to find

        for (let j = 2; j <= 9; j++) { //effects, value, weight and rarity
            sel[j] = `#mw-content-text > table:nth-child(14) > tbody > tr:nth-child(${i+1}) > td:nth-child(${j-1})`;

            if (j >= 2 && j <= 5) { //processing coefficient of ingrident magnitude, duration and value


                for (k = 4; k <= 6; k++) {

                    let sel_coef = `#mw-content-text > table:nth-child(14) > tbody > tr:nth-child(${i+1}) > td:nth-child(${j - 1}) > span:nth-child(${k-1}) > b`,
                        sel_act = `#mw-content-text > table:nth-child(14) > tbody > tr:nth-child(${i+1}) > td:nth-child(${j - 1}) > a:nth-child(${k}) > img`;

                    switch ($(sel_act).attr('alt')) {
                        case 'Magnitude':
                            mag[j - 2] = $(sel_coef).text();
                            break;
                        case 'Duration':
                            dur[j - 2] = $(sel_coef).text();
                            break;
                        case 'Value':
                            val[j - 2] = $(sel_coef).text();
                            break;
                    }

                }

            }


        }

        sel[10] = `#mw-content-text > table:nth-child(14) > tbody > tr:nth-child(2) > td:nth-child(1) > a > img`; //image
        sel[11] = `#mw-content-text > table:nth-child(14) > tbody > tr:nth-child(2) > td:nth-child(2) > span > span`; //id


        let ingredient = {
            id: $(sel[11]).text(),
            name: $(sel[0]).text(),
            acquiredBy: $(sel[1]).text(),
            effects: [
                $(sel[2] + ` > a`).attr('title'),
                $(sel[3] + ` > a`).attr('title'),
                $(sel[4] + ` > a`).attr('title'),
                $(sel[5] + ` > a`).attr('title')
            ],
            effectMag: mag,
            effectDur: dur,
            effectVal: val,
            value: $(sel[6]).text(),
            weight: $(sel[7]).text(),
            rarity: $(sel[8]).text(),
            garden: $(sel[9]).text()

        };

        Ingredients.push(ingredient);
    }

    fs.writeFileSync('./data/tesv_main.json', JSON.stringify(Ingredients).replace(/\ /g, ''));

    const IngredientsCC = new Array;

    console.log('Extracting CC creations ingredients...');

    for (let i = 2; i <= 200; i += 2) { //for main table table[14], for CC - table[20]
        let sel = new Array;
        sel[0] = `#mw-content-text > table:nth-child(20) > tbody > tr:nth-child(${i}) > td:nth-child(2) > a`; //name

        if ($(sel[0]) == "")
            break;

        let mag = new Array(4).fill(null),
            dur = new Array(4).fill(null),
            val = new Array(4).fill(null);

        sel[1] = `#mw-content-text > table:nth-child(20) > tbody > tr:nth-child(${i}) > td:nth-child(3)`; //where to find

        for (let j = 2; j <= 9; j++) { //effects, value, weight and rarity
            sel[j] = `#mw-content-text > table:nth-child(20) > tbody > tr:nth-child(${i+1}) > td:nth-child(${j-1})`;

            if (j >= 2 && j <= 5) { //processing coefficient of ingrident magnitude, duration and value


                for (k = 4; k <= 6; k++) {

                    let sel_coef = `#mw-content-text > table:nth-child(20) > tbody > tr:nth-child(${i+1}) > td:nth-child(${j - 1}) > span:nth-child(${k-1}) > b`,
                        sel_act = `#mw-content-text > table:nth-child(20) > tbody > tr:nth-child(${i+1}) > td:nth-child(${j - 1}) > a:nth-child(${k}) > img`;

                    switch ($(sel_act).attr('alt')) {
                        case 'Magnitude':
                            mag[j - 2] = $(sel_coef).text();
                            break;
                        case 'Duration':
                            dur[j - 2] = $(sel_coef).text();
                            break;
                        case 'Value':
                            val[j - 2] = $(sel_coef).text();
                            break;
                    }

                }

            }


        }

        sel[10] = `#mw-content-text > table:nth-child(20) > tbody > tr:nth-child(2) > td:nth-child(1) > a > img`; //image
        sel[11] = `#mw-content-text > table:nth-child(20) > tbody > tr:nth-child(2) > td:nth-child(2) > span > span`; //id


        let ingredient = {
            id: $(sel[11]).text(),
            name: $(sel[0]).text(),
            acquiredBy: $(sel[1]).text(),
            effects: [
                $(sel[2] + ` > a`).attr('title'),
                $(sel[3] + ` > a`).attr('title'),
                $(sel[4] + ` > a`).attr('title'),
                $(sel[5] + ` > a`).attr('title')
            ],
            effectMag: mag,
            effectDur: dur,
            effectVal: val,
            value: $(sel[6]).text(),
            weight: $(sel[7]).text(),
            rarity: $(sel[8]).text(),
            garden: $(sel[9]).text()

        };

        Ingredients.push(ingredient);
        IngredientsCC.push(ingredient);

    }

    fs.writeFileSync('./data/tesv_cc.json', JSON.stringify(IngredientsCC).replace(/\ /g, ''));
    fs.writeFileSync('./data/tesv_merged.json', JSON.stringify(Ingredients).replace(/\ /g, ''));

}


main();