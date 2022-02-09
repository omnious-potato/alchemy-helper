const cheerio = require('cheerio');
const request = require('request-promise');
const yargs = require('yargs');
const fs = require('fs');


const local_path = './presaved/tesv_effects.html';
const uesp_link = 'https://en.uesp.net/wiki/Skyrim:Alchemy_Effects#Effect_List';

let argv = yargs.argv;
console.log(argv);


const tesv_ingredients


async function main() {

    if (argv._.includes('online')) {

        console.log("Loading HTML from UESP!");

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

    const Effects = new Array;



    //console.log($(`#mw-content-text > table > tbody > tr:nth-child(${1}) > th > a`).text());

    
    for (let i = 2; i <= 70; i++) {
        let sel = new Array;
        sel[0] = `#mw-content-text > table > tbody > tr:nth-child(${i}) > th > a`;
        
        if($(sel[0]) == '')
            break;

        sel[1] = `#mw-content-text > table > tbody > tr:nth-child(${i}) > th > span.idall > span`

        for (let j = 2; j <= 5; j++) {
            sel[j] = `#mw-content-text > table > tbody > tr:nth-child(${i}) > td:nth-child(${j+2})`;
        }

        sel[6] = `#mw-content-text > table > tbody > tr:nth-child(${i}) > th `;

        let effect = {
            id: $(sel[1]).text(),
            name: $(sel[0]).text(),
            isPositive: ($(sel[6]).attr('class') == 'EffectPos' ? true : false  ),
            Base_Cost: parseFloat($(sel[2]).text()),
            Base_Mag: parseFloat($(sel[3]).text()),
            Base_Dur: parseFloat($(sel[4]).text()),
            value: parseFloat($(sel[5]).text())

        }

        Effects.push(effect);

    }

    fs.writeFileSync('./data/tesv_effects.json', JSON.stringify(Effects));

    console.log(Effects);

}

main();