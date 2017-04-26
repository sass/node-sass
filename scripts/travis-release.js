const fs = require('fs');

fs.readdir('vendor/', (err, list) => {
    const dirName = list[0];
    const fileName = `${dirName}_binding.node`;
    const filePath = `vendor/${dirName}/binding.node`;

    fs.createReadStream(filePath).pipe(fs.createWriteStream(fileName));

    return console.log(fileName);
});
