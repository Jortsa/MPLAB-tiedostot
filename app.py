from flask import Flask, render_template, request, jsonify # kirjastot
import requests # http-pyynnöt

app = Flask(__name__) # luo flask sovelluksen

DISCOGS_TOKEN = 'capvToXZPfdCRBrKBgiobFRPRpKVJBHakbHfNhiq' # API-avain
HEADERS = {'Authorization': f'Discogs token={DISCOGS_TOKEN}'} # lähetetään api-pyynnöissä

# palauttaa html sivun
@app.route('/')
def index():
    return render_template('index.html') # näytetään pääsivu


@app.route('/hae_hinta', methods=['POST']) # "reitti" hinnan haulle
def hae_hinta():
    release_id = request.form['release_id'] # tallennetaan levy muuttujaan
    stats_url = f'https://api.discogs.com/marketplace/stats/{release_id}' # luodaan haku url
    stats = requests.get(stats_url, headers=HEADERS).json() # haetaan tiedot ja muunnetaan json
    price = stats.get('lowest_price') # marketplace:n halvin hinta kyseisestä levystä

    if price is None: # jos hintaa ei löydetty, palautetaan virheviesti
        return jsonify({'hinta': 'Levyä ei juuri nyt ole myynnissä'})
    
    return jsonify({'hinta': price})


@app.route('/hae_painokset', methods=['POST']) # "reitti" painoksen hakemiselle
def hae_painokset():

    # tallennetaan syötteenä saadut tiedot muuttujiin
    artisti = request.form['artisti']
    levy = request.form['levy']
    formaatti = request.form['formaatti']

    haku_url = f'https://api.discogs.com/database/search?q={artisti}+{levy}&type=release&format={formaatti}' # luodaan haku url
    haku = requests.get(haku_url, headers=HEADERS).json() # haetaan tiedot ja muunnetaan json

    painokset = [] # lista löydetyistä painoksista
    for r in haku.get('results', []): # käydään läpi löydetyt tulokset
        painos = {
            'id': r['id'], # painoksen ID, joka näkyy http-osoitteessa viimeisenä
            'title': r['title'], # levyn nimi
            'year': r.get('year', 'N/A'), # julkaisuvuosi
            'country': r.get('country', 'N/A'), # painoksen maa
            'format': ', '.join(r.get('format', [])), # formaatti (lp/cd/kas)
            'label': ', '.join(r.get('label', [])) if 'label' in r else 'N/A', # levy-yhtiö
            'catno': r.get('catno', 'N/A') # tunniste / julkaisun numero
        }
        painokset.append(painos) # lisätään painokset-listaan

    return jsonify({'painokset': painokset}) # palauttaa painokset


if __name__ == '__main__':
    app.run(debug=True)
