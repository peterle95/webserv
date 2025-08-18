/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   ErrorHandler.hpp                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: pmolzer <pmolzer@student.42berlin.de>      +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2025/08/18 11:08:47 by pmolzer           #+#    #+#             */
/*   Updated: 2025/08/18 11:08:47 by pmolzer          ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef ERRORHANDLER_HPP
#define ERRORHANDLER_HPP

class ErrorHandler 
{
    private:

    public:
    ErrorHandler();
    ~ErrorHandler();

    class Error : public std::execption
    {
        public:
            const char* what() const throw();
    };
};

#endif

