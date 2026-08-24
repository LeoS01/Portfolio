export async function GETAll(fileTypes){
    let allElements = []
    for(const type of fileTypes){
        //Fetch Paths
        const files = await fetch(type);
        if(!files) continue;

        const allFiles = await files.json();
        allFiles.reverse();
        
        for(const e of allFiles){
            allElements.push(e);
        }
    }

    return allElements;
}


export async function PUTData(data, uri){
    const prms = fetch(uri, {method: "PUT", body: data});
    if(!prms) return false;
    await prms;
    return true;
}