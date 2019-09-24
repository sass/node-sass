const fs = require('fs');

var dir = 'release';

if (!fs.existsSync(dir)){
    fs.mkdirSync(dir);
}

fs.readdir('vendor/', (err, list) => {
    const dirName = list[0];
    const fileName = `${dirName}_binding.node`;
    const filePath = `vendor/${dirName}/binding.node`;

    fs.createReadStream(filePath).pipe(fs.createWriteStream(`${dir}/${fileName}`));

    return console.log(fileName);
});
