// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

// Copyright 2019 John Maloney, Bernat Romagosa, and Jens Mönig

// locale.gp
// Bernat Romagosa, January 2020
//
// Update missing strings in locale files

to startup {
	langName = (last (commandLine))
	setLanguage (authoringSpecs) langName

        // Backup previous locale file before updating it
	oldLocale = (readFile (join '../translations/' langName '.txt'))
	(writeFile (join (tmpPath) langName '.txt') oldLocale)
	updatedLocale = ''

	allLocales = (readFile '../Locales.txt')

	lines = (toList (lines allLocales))
	while ((count lines) >= 1) {
		original = (removeFirst lines)
                if (or (beginsWith original '#') (original == '')) {
			// Copy comments. We should be smarter about it and get
			// them from the original file somehow.
			updatedLocale = (join updatedLocale original (newline))
                } else {
			translation = (localizedOrNil original)
			if (isNil translation) { translation = '--MISSING--' }
			updatedLocale = (join
				updatedLocale
				original
				(newline)
				translation
				(newline))
		}
	}

	writeFile (join '../translations/' langName '.txt') updatedLocale
}

to tmpPath {
	if (or ('Mac' == (platform)) ('Linux' == (platform))) {
		return '/tmp/'
	} else { // Windows
		return (join (userHomePath) '/AppData/Local/Temp/')
	}
}
